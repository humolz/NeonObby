#include "net/Updater.h"
#include "Version.h"

// Pure WinHTTP implementation — no curl/libcurl dependency, no extra DLLs.
// WinHTTP ships with Windows since XP, supports TLS 1.2/1.3, and follows
// the same protocol GitHub's API requires.
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>

#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>
#include <filesystem>

#pragma comment(lib, "winhttp.lib")

namespace Updater {

namespace {

// =============================================================================
// CONFIGURATION — change these once you publish on GitHub.
//
// To enable updates:
//   1. Create a public GitHub repo (e.g. github.com/your-username/NeonObby).
//   2. Replace kGithubOwner and kGithubRepo below with the real values.
//   3. Build a release: NeonObbySetup-X.Y.Z.exe (matches kInstallerAssetPrefix).
//   4. On GitHub, draft a release with tag "vX.Y.Z" and attach the installer.
//   5. Bump project(NeonObby VERSION X.Y.Z) in CMakeLists.txt before building
//      the *new* version that should be offered to existing players.
//
// Until kGithubOwner is replaced, checkForUpdate() will quietly fail with
// kUpdatesDisabled — the UI just stays hidden and nothing breaks.
// =============================================================================
constexpr const char* kGithubOwner = "humolz";
constexpr const char* kGithubRepo  = "NeonObby";
constexpr const char* kInstallerAssetPrefix = "NeonObbySetup-";
constexpr const char* kInstallerAssetSuffix = ".exe";
constexpr const char* kUserAgent = "NeonObby-Updater/1.0";

// -----------------------------------------------------------------------------
// Shared state guarded by g_mutex. The background thread writes; the UI thread
// reads via getStatus(). Atomics for the cheap fields, mutex for the strings.
// -----------------------------------------------------------------------------
std::mutex g_mutex;
Status g_status;
std::thread g_worker;
std::atomic<bool> g_workerBusy{false};

void setState(State newState) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_status.state = newState;
}

void setError(const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_status.state = State::Failed;
    g_status.error = msg;
}

// -----------------------------------------------------------------------------
// Convert UTF-8 → UTF-16 for WinHTTP (which is wide-char only).
// -----------------------------------------------------------------------------
std::wstring toWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len > 0 ? len - 1 : 0, L'\0');
    if (len > 0) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    }
    return out;
}

// -----------------------------------------------------------------------------
// Parse a URL like "https://api.github.com/repos/foo/bar" into host + path.
// Returns false on malformed input. Used so we can re-resolve after redirects.
// -----------------------------------------------------------------------------
struct ParsedUrl {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
    bool https = true;
};

bool parseUrl(const std::wstring& url, ParsedUrl& out) {
    URL_COMPONENTSW uc{};
    uc.dwStructSize     = sizeof(uc);
    wchar_t hostBuf[256] = {};
    wchar_t pathBuf[2048] = {};
    uc.lpszHostName     = hostBuf;
    uc.dwHostNameLength = static_cast<DWORD>(std::size(hostBuf));
    uc.lpszUrlPath      = pathBuf;
    uc.dwUrlPathLength  = static_cast<DWORD>(std::size(pathBuf));

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return false;

    out.host  = std::wstring(uc.lpszHostName, uc.dwHostNameLength);
    out.path  = std::wstring(uc.lpszUrlPath,  uc.dwUrlPathLength);
    out.port  = uc.nPort;
    out.https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    return true;
}

// -----------------------------------------------------------------------------
// HTTPS GET → byte buffer. Used both for the JSON release feed and the
// installer download. Reports progress via the optional callback so the
// download UI can show a live percentage.
//
// Handles redirects automatically (GitHub Releases assets redirect from
// api.github.com to objects.githubusercontent.com on download).
// -----------------------------------------------------------------------------
using ProgressCb = void(*)(size_t bytesSoFar, size_t totalBytes);

bool httpsGet(const std::wstring& url, std::vector<char>& out,
              const std::wstring& accept, ProgressCb onProgress) {
    ParsedUrl parsed;
    if (!parseUrl(url, parsed)) return false;

    HINTERNET session = WinHttpOpen(toWide(kUserAgent).c_str(),
                                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;

    // Always follow redirects — GitHub uses 302s for asset downloads.
    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));

    HINTERNET conn = WinHttpConnect(session, parsed.host.c_str(), parsed.port, 0);
    if (!conn) { WinHttpCloseHandle(session); return false; }

    DWORD flags = parsed.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET req = WinHttpOpenRequest(conn, L"GET", parsed.path.c_str(),
                                       nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!req) { WinHttpCloseHandle(conn); WinHttpCloseHandle(session); return false; }

    // Add Accept header (GitHub API needs application/vnd.github+json).
    if (!accept.empty()) {
        std::wstring header = L"Accept: " + accept;
        WinHttpAddRequestHeaders(req, header.c_str(), (DWORD)-1,
                                 WINHTTP_ADDREQ_FLAG_ADD);
    }

    bool ok = WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
           && WinHttpReceiveResponse(req, nullptr);

    if (ok) {
        // Check status code — must be 200, otherwise treat as failure.
        DWORD statusCode = 0;
        DWORD szSC = sizeof(statusCode);
        WinHttpQueryHeaders(req,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &szSC, WINHTTP_NO_HEADER_INDEX);
        if (statusCode != 200) {
            ok = false;
        }
    }

    // Try to discover content length so we can compute download %.
    size_t totalBytes = 0;
    if (ok) {
        wchar_t lenBuf[64] = {};
        DWORD lenSz = sizeof(lenBuf);
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH,
                                WINHTTP_HEADER_NAME_BY_INDEX, lenBuf, &lenSz,
                                WINHTTP_NO_HEADER_INDEX)) {
            totalBytes = static_cast<size_t>(_wtoi64(lenBuf));
        }
    }

    if (ok) {
        out.clear();
        out.reserve(totalBytes > 0 ? totalBytes : 64 * 1024);
        DWORD avail = 0;
        do {
            avail = 0;
            if (!WinHttpQueryDataAvailable(req, &avail)) { ok = false; break; }
            if (avail == 0) break;

            size_t oldSize = out.size();
            out.resize(oldSize + avail);
            DWORD read = 0;
            if (!WinHttpReadData(req, out.data() + oldSize, avail, &read)) {
                ok = false; break;
            }
            out.resize(oldSize + read);

            if (onProgress) onProgress(out.size(), totalBytes);
        } while (avail > 0);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    WinHttpCloseHandle(session);
    return ok;
}

// -----------------------------------------------------------------------------
// Version comparison. Both inputs are dotted decimal — leading "v" tolerated.
// Returns true if `latest` is strictly newer than `current`.
// -----------------------------------------------------------------------------
std::tuple<int, int, int> parseVersion(const std::string& s) {
    int parts[3] = {0, 0, 0};
    const char* p = s.c_str();
    if (*p == 'v' || *p == 'V') ++p;
    int idx = 0;
    while (*p && idx < 3) {
        if (*p >= '0' && *p <= '9') {
            parts[idx] = parts[idx] * 10 + (*p - '0');
        } else if (*p == '.') {
            ++idx;
        } else {
            break;
        }
        ++p;
    }
    return {parts[0], parts[1], parts[2]};
}

bool isNewer(const std::string& latest, const std::string& current) {
    return parseVersion(latest) > parseVersion(current);
}

// -----------------------------------------------------------------------------
// Where to drop the downloaded installer. Use %TEMP% so Windows cleans it up
// eventually and we don't write into Program Files (which we may not own).
// -----------------------------------------------------------------------------
std::string getDownloadPath(const std::string& version) {
    char tempDir[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempDir);
    std::string base = (len > 0 && len < MAX_PATH) ? std::string(tempDir, len) : std::string(".\\");
    return base + std::string(kInstallerAssetPrefix) + version + std::string(kInstallerAssetSuffix);
}

// -----------------------------------------------------------------------------
// Quick guard so we don't spawn checks against the placeholder repo.
// -----------------------------------------------------------------------------
bool updatesEnabled() {
    return std::string(kGithubOwner) != "YOUR_GITHUB_USER";
}

// -----------------------------------------------------------------------------
// Worker bodies. Each one runs on g_worker — never call directly from the
// main thread because they perform network I/O.
// -----------------------------------------------------------------------------
void workerCheck() {
    if (!updatesEnabled()) {
        // Stay silent. UI checks for UpToDate-or-Idle to decide visibility.
        setState(State::Idle);
        g_workerBusy = false;
        return;
    }

    setState(State::Checking);

    std::string apiUrl = std::string("https://api.github.com/repos/")
                       + kGithubOwner + "/" + kGithubRepo + "/releases/latest";

    std::vector<char> body;
    if (!httpsGet(toWide(apiUrl), body, L"application/vnd.github+json", nullptr)) {
        setError("Failed to reach GitHub release feed");
        g_workerBusy = false;
        return;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(body.begin(), body.end());
        std::string tag = j.value("tag_name", "");
        if (tag.empty()) { setError("Release feed has no tag_name"); g_workerBusy = false; return; }

        // Pick the .exe asset whose name starts with kInstallerAssetPrefix.
        std::string downloadUrl;
        if (j.contains("assets") && j["assets"].is_array()) {
            for (const auto& asset : j["assets"]) {
                std::string name = asset.value("name", "");
                if (name.rfind(kInstallerAssetPrefix, 0) == 0
                    && name.size() >= 4
                    && name.substr(name.size() - 4) == kInstallerAssetSuffix) {
                    downloadUrl = asset.value("browser_download_url", "");
                    break;
                }
            }
        }
        if (downloadUrl.empty()) {
            setError("Latest release has no installer asset");
            g_workerBusy = false;
            return;
        }

        std::string current = NeonObby::kVersionString;
        if (isNewer(tag, current)) {
            std::lock_guard<std::mutex> lk(g_mutex);
            g_status.state = State::UpdateAvailable;
            g_status.latestVersion = tag;
            g_status.downloadUrl = downloadUrl;
            g_status.error.clear();
        } else {
            setState(State::UpToDate);
        }
    } catch (const std::exception& e) {
        setError(std::string("Bad release JSON: ") + e.what());
    }

    g_workerBusy = false;
}

void workerDownload() {
    Status snap;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        snap = g_status;
    }
    if (snap.state != State::UpdateAvailable && snap.state != State::Failed) {
        g_workerBusy = false;
        return;
    }

    setState(State::Downloading);
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_status.downloadProgress = 0.0f;
    }

    std::vector<char> data;
    bool ok = httpsGet(toWide(snap.downloadUrl), data, L"application/octet-stream",
        [](size_t soFar, size_t total) {
            float pct = total > 0 ? float(double(soFar) / double(total)) : 0.0f;
            std::lock_guard<std::mutex> lk(g_mutex);
            g_status.downloadProgress = pct;
        });

    if (!ok || data.empty()) {
        setError("Installer download failed");
        g_workerBusy = false;
        return;
    }

    std::string outPath = getDownloadPath(snap.latestVersion);
    try {
        std::ofstream f(outPath, std::ios::binary);
        if (!f) { setError("Cannot write installer to " + outPath); g_workerBusy = false; return; }
        f.write(data.data(), static_cast<std::streamsize>(data.size()));
    } catch (const std::exception& e) {
        setError(std::string("Write failed: ") + e.what());
        g_workerBusy = false;
        return;
    }

    {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_status.state = State::ReadyToInstall;
        g_status.downloadProgress = 1.0f;
        g_status.error.clear();
    }
    g_workerBusy = false;
}

// Join the previous worker (if any) before spawning the next one.
void joinWorker() {
    if (g_worker.joinable()) g_worker.join();
}

} // namespace

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void checkForUpdate() {
    bool expected = false;
    if (!g_workerBusy.compare_exchange_strong(expected, true)) return;
    joinWorker();
    g_worker = std::thread(workerCheck);
}

void downloadUpdate() {
    bool expected = false;
    if (!g_workerBusy.compare_exchange_strong(expected, true)) return;
    joinWorker();
    g_worker = std::thread(workerDownload);
}

bool applyUpdate() {
    Status snap;
    {
        std::lock_guard<std::mutex> lk(g_mutex);
        snap = g_status;
    }
    if (snap.state != State::ReadyToInstall) return false;

    std::string installer = getDownloadPath(snap.latestVersion);
    if (!std::filesystem::exists(installer)) {
        setError("Downloaded installer missing");
        return false;
    }

    // Inno Setup silent flags:
    //   /SILENT             — suppress wizard pages, show only progress bar
    //   /SUPPRESSMSGBOXES   — auto-answer info dialogs
    //   /CURRENTUSER        — install per-user (matches our installer mode)
    //   /NORESTART          — never reboot the machine
    // The script's [Setup] CloseApplications=force will gracefully close us.
    std::string cmdLine = "\"" + installer + "\" /SILENT /SUPPRESSMSGBOXES /CURRENTUSER /NORESTART";

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
                             0, nullptr, nullptr, &si, &pi);
    if (!ok) {
        setError("Failed to launch installer");
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

Status getStatus() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_status;
}

void shutdown() {
    joinWorker();
}

} // namespace Updater
