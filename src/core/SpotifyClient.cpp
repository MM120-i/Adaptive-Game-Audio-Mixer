#include "SpotifyClient.h"
#include "spotify_config.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_events/juce_events.h>

namespace {
    constexpr int CONNECTION_TIMEOUT_MS = 5000;
    constexpr int CALLBACK_PORT = 8888;
    constexpr int BUFFER_SIZE = 4096;

    juce::String loadAuthPage(const char *filename){
        auto dir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("auth");
        auto html = dir.getChildFile(filename).loadFileAsString();
        auto css = dir.getChildFile("style.css").loadFileAsString();

        return html.replace("</title>", "</title><style>" + css + "</style>", false);
    }
}

// PCKE Code verifier =====================
juce::String SpotifyClient::generateCodeVerifier(){
    juce::Random rng;
    juce::String verifier;

    static constexpr auto chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";

    for(size_t i = 0; i < 64; i++)
        verifier += chars[rng.nextInt(66)];
    
    return verifier;
}

juce::String SpotifyClient::generateCodeChallenge(const juce::String &verifier){
    juce::SHA256 hasher(verifier.toUTF8());
    const auto hash = hasher.getRawData();

    return juce::Base64::toBase64(hash.getData(), hash.getSize())
                .replace("+", "-").replace("/", "_").replace("=", "");
}

// HTTP Helpers =====================
juce::String SpotifyClient::httpGet(const juce::String &url, const juce::StringArray &extraHeaders){
    juce::URL req(url);

    auto base = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(CONNECTION_TIMEOUT_MS)
                    .withHttpRequestCmd("GET");

    const auto options = extraHeaders.isEmpty() 
               ? base 
               : base.withExtraHeaders(extraHeaders.joinIntoString("\r\n"));
    
    auto stream = req.createInputStream(options);

    if(!stream)
        return {};

    return stream->readEntireStreamAsString();
}

juce::var SpotifyClient::httpPostForm(
    const juce::String &url,
    const juce::StringPairArray &formData,
    const juce::StringArray &extraHeaders
){
    juce::StringArray pairs;
    
    for(const auto &key : formData.getAllKeys())
        pairs.add(juce::URL::addEscapeChars(key, true) + "=" + juce::URL::addEscapeChars(formData[key], true));

    const auto body = pairs.joinIntoString("&");
    auto allHeaders = juce::StringArray("Content-Type: application/x-www-form-urlencoded");

    for(const auto &h : extraHeaders)
        allHeaders.add(h);

    juce::URL req(juce::URL(url).withPOSTData(body));
    
    auto base = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(CONNECTION_TIMEOUT_MS)
                    .withHttpRequestCmd("POST")
                    .withExtraHeaders(allHeaders.joinIntoString("\r\n"));

    auto stream = req.createInputStream(base);

    if(!stream)
        return juce::var();

    const auto response = stream->readEntireStreamAsString();

    if(response.isEmpty())
        return juce::var();

    return juce::JSON::parse(response);
}

juce::String SpotifyClient::apiGet(const juce::String &path){
    if(accessToken.isEmpty())
        return {};

    return httpGet(kApiBase + path, juce::StringArray("Authorization: Bearer " + accessToken));
}

juce::var SpotifyClient::apiPost(
    const juce::String &path,
    const juce::StringPairArray &body,
    const juce::StringArray &extraHeaders
){
    if(accessToken.isEmpty())
        return juce::var();

    auto headers = juce::StringArray("Authorization: Bearer " + accessToken);

    for(const auto &header : extraHeaders)
        headers.add(header);

    return httpPostForm(kApiBase + path, body, headers);
}


// Constructor / Destructor =====================
SpotifyClient::SpotifyClient() = default;

SpotifyClient::~SpotifyClient(){
    disconnect();

    if(serverThread.joinable())
        serverThread.join();
}

// Auth ===================== 
bool SpotifyClient::isAuthenticated() const {
    return authenticated;
}

void SpotifyClient::startAuth(){
    {
        const juce::ScopedLock sl(lock);
        codeVerifier = generateCodeVerifier();
        status_ = SpotifyStatus::Connecting;
        lastErrorMessage_.clear();
    }

    const auto challenge = generateCodeChallenge(codeVerifier);
    juce::URL authUrl(kAuthorizeUrl);

    authUrl = authUrl.withParameter("client_id", SPOTIFY_CLIENT_ID);
    authUrl = authUrl.withParameter("response_type", "code");
    authUrl = authUrl.withParameter("redirect_uri", SPOTIFY_REDIRECT_URI);
    authUrl = authUrl.withParameter("code_challenge_method", "S256");
    authUrl = authUrl.withParameter("code_challenge", challenge);
    authUrl = authUrl.withParameter("scope", kScopes);

    authUrl.launchInDefaultBrowser();
    startCallbackServer();
}

void SpotifyClient::disconnect(){
    const juce::ScopedLock sl(lock);

    if(serverSocketHandle){
        closesocket((SOCKET)serverSocketHandle);
        serverSocketHandle = 0;
    }

    authenticated = false;
    accessToken.clear();
    refreshToken.clear();
    tokenExpiry = 0;
    playing = false;
    currentTrack.clear();
    currentArtist.clear();
    currentAlbum.clear();
    currentAlbumArtUrl.clear();
    currentDeviceName.clear();
    currentVolume = 0;
    status_ = SpotifyStatus::Disconnected;
    lastErrorMessage_.clear();
    deviceActive_ = false;
}

void SpotifyClient::loadTokens(const AppSettings &settings){
    bool needsRefresh = false;

    {
        const juce::ScopedLock sl(lock);

        accessToken = settings.spotifyAccessToken;
        refreshToken = settings.spotifyRefreshToken;
        tokenExpiry = settings.spotifyTokenExpiry;

        const auto now = juce::Time::currentTimeMillis();

        if(refreshToken.isNotEmpty() && tokenExpiry > 0){
            if(now >= tokenExpiry)
                needsRefresh = true;
            else
                authenticated = true;
        }
    }

    if(needsRefresh){
        refreshAccessToken();
    }
    else {
        const juce::ScopedLock sl(lock);

        if(authenticated)
            status_ = SpotifyStatus::Connected;
    }
}

void SpotifyClient::saveTokens(AppSettings &settings) const {
    const juce::ScopedLock sl(lock);

    settings.spotifyAccessToken = accessToken;
    settings.spotifyRefreshToken = refreshToken;
    settings.spotifyTokenExpiry = tokenExpiry;
}

// Token Exchange + refresh =====================
void SpotifyClient::refreshAccessToken(){
    juce::String localRefreshToken;

    {
        const juce::ScopedLock sl(lock);
        localRefreshToken = refreshToken;
    }

    juce::StringPairArray form;
    form.set("grant_type", "refresh_token");
    form.set("refresh_token", localRefreshToken);
    form.set("client_id", SPOTIFY_CLIENT_ID);

    const auto json = httpPostForm(kTokenUrl, form);
    const juce::ScopedLock sl(lock);

    if(!json.isObject()){
        authenticated = false;
        status_ = SpotifyStatus::Error;
        lastErrorMessage_ = "Token refresh failed - please reconnect";
        return;
    }

    const auto *obj = json.getDynamicObject();

    if(!obj || !obj->hasProperty("access_token")){
        authenticated = false;
        status_ = SpotifyStatus::Error;
        return;
    }

    accessToken = obj->getProperty("access_token").toString();
    const auto expiresIn = static_cast<int>(obj->getProperty("expires_in"));
    tokenExpiry = juce::Time::currentTimeMillis() + (expiresIn - 30) * 1000;

    if(obj->hasProperty("refresh_token"))
        refreshToken = obj->getProperty("refresh_token").toString();

    authenticated = true;
    status_ = SpotifyStatus::Connected;
}

void SpotifyClient::exchangeCodeForTokens(const juce::String &code){
    juce::String localVerifier;

    {
        const juce::ScopedLock sl(lock);
        localVerifier = codeVerifier;
        codeVerifier.clear();
    }

    auto body = juce::String("grant_type=authorization_code")
                + "&code=" + code
                + "&redirect_uri=" + SPOTIFY_REDIRECT_URI
                + "&client_id=" + SPOTIFY_CLIENT_ID
                + "&code_verifier=" + localVerifier;

    juce::URL req(kTokenUrl);
    req = req.withPOSTData(body);

    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(CONNECTION_TIMEOUT_MS)
                       .withHttpRequestCmd("POST")
                       .withExtraHeaders("Content-Type: application/x-www-form-urlencoded");

    std::unique_ptr<juce::InputStream> stream(req.createInputStream(options));

    if(!stream){
        const juce::ScopedLock sl(lock);
        status_ = SpotifyStatus::Error;
        lastErrorMessage_ = "Token endpoint unreachable";
        return;
    }

    juce::String response = stream->readEntireStreamAsString();
    juce::var json = juce::JSON::parse(response);

    if(!json.isObject()){
        const juce::ScopedLock sl(lock);
        status_ = SpotifyStatus::Error;
        lastErrorMessage_ = "Token endpoint returned invalid response: " + response.substring(0, 100);

        return;
    }

    juce::DynamicObject* obj = json.getDynamicObject();

    if(!obj){
        const juce::ScopedLock sl(lock);
        status_ = SpotifyStatus::Error;
        lastErrorMessage_ = "Token endpoint returned empty response";
        
        return;
    }

    {
        const juce::ScopedLock sl(lock);

        accessToken = obj->getProperty("access_token").toString();
        refreshToken = obj->getProperty("refresh_token").toString();
        tokenExpiry = juce::Time::currentTimeMillis()
                      + (static_cast<int>(obj->getProperty("expires_in")) - 30) * 1000;
        authenticated = !accessToken.isEmpty();

        if(authenticated){
            status_ = SpotifyStatus::Connected;
        }
        else {
            status_ = SpotifyStatus::Error;

            if(obj->hasProperty("error"))
                lastErrorMessage_ = "Spotify: " + obj->getProperty("error").toString();

            if(obj->hasProperty("error_description"))
                lastErrorMessage_ += " - " + obj->getProperty("error_description").toString();
        }
    }
}

// Callback Server =====================
void SpotifyClient::startCallbackServer(){
    if(serverThread.joinable())
        serverThread.join();

    serverThread = std::thread([this] {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        auto listenSocket = socket(AF_INET, SOCK_STREAM, 0);

        if(listenSocket == INVALID_SOCKET){
            const juce::ScopedLock sl(lock);
            status_ = SpotifyStatus::Error;
            lastErrorMessage_ = "Failed to create socket";

            if(onStateChanged) 
                juce::MessageManager::callAsync(onStateChanged);
            
            return;
        }

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons((u_short)CALLBACK_PORT);

        if(bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
            closesocket(listenSocket);
            const juce::ScopedLock sl(lock);
            status_ = SpotifyStatus::Error;
            lastErrorMessage_ = "Failed to bind port " + juce::String(CALLBACK_PORT);

            if(onStateChanged) 
                juce::MessageManager::callAsync(onStateChanged);

            return;
        }

        if(listen(listenSocket, SOMAXCONN) == SOCKET_ERROR){
            closesocket(listenSocket);
            const juce::ScopedLock sl(lock);
            status_ = SpotifyStatus::Error;
            lastErrorMessage_ = "Failed to listen on port " + juce::String(CALLBACK_PORT);

            if(onStateChanged)
                juce::MessageManager::callAsync(onStateChanged);

            return;
        }

        {
            const juce::ScopedLock sl(lock);
            serverSocketHandle = (uintptr_t)listenSocket;
        }

        auto clientSocket = accept(listenSocket, nullptr, nullptr);
        closesocket(listenSocket);

        {
            const juce::ScopedLock sl(lock);
            serverSocketHandle = 0;
        }

        if(clientSocket == INVALID_SOCKET){
            if(onStateChanged) 
                juce::MessageManager::callAsync(onStateChanged);

            return;
        }

        char buf[BUFFER_SIZE] = {};
        recv(clientSocket, buf, sizeof(buf) - 1, 0);
        juce::String request(buf);
        const auto codeStart = request.indexOf("?code=");

        if(codeStart != -1){
            const auto codeEnd = request.indexOf(codeStart + 6, " ");
            const auto code = request.substring(
                codeStart + 6, 
                codeEnd == -1 ? codeStart + 200 : codeEnd
            );

            exchangeCodeForTokens(code);

            const juce::String response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Connection: close\r\n"
                                   "\r\n"
                                   + loadAuthPage("success.html");

            send(clientSocket, response.getCharPointer(), response.length(), 0);
        } 
        else {
            const juce::String response = "HTTP/1.1 400 Bad Request\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Connection: close\r\n"
                                   "\r\n"
                                   + loadAuthPage("error.html");

            send(clientSocket, response.getCharPointer(), response.length(), 0);
        }

        closesocket(clientSocket);

        if(onStateChanged)
            juce::MessageManager::callAsync(onStateChanged);
    });
}

// Polling =====================
void SpotifyClient::poll(){
    juce::String localToken;
    bool needsRefresh = false;

    {
        const juce::ScopedLock sl(lock);

        if(!authenticated)
            return;

        const auto now = juce::Time::currentTimeMillis();

        if(now >= tokenExpiry)
            needsRefresh = true;

        localToken = accessToken;
    }

    if(needsRefresh){
        refreshAccessToken();

        {
            const juce::ScopedLock sl(lock);

            if(!authenticated) 
                return;

            localToken = accessToken;
        }
    }

    bool networkOk = true;

    const auto playbackJson = httpGet(
        juce::String(kApiBase) + "/me/player/currently-playing",
        juce::StringArray("Authorization: Bearer " + localToken)
    );

    if(playbackJson.isNotEmpty()){
        const auto parsed = juce::JSON::parse(playbackJson);

        if(parsed.isObject()){
            const juce::ScopedLock sl(lock);
            parsePlaybackState(parsed);
        }
    }
    else{
        networkOk = false;
    }

    const auto devicesJson = httpGet(
        juce::String(kApiBase) + "/me/player/devices",
        juce::StringArray("Authorization: Bearer " + localToken)
    );

    if(devicesJson.isNotEmpty()){
        const auto parsed = juce::JSON::parse(devicesJson);

        if(parsed.isObject()){
            const juce::ScopedLock sl(lock);
            parseDevices(parsed);
        }
    }
    else{
        networkOk = false;
    }

    {
        const juce::ScopedLock sl(lock);

        if(!networkOk){
            status_ = SpotifyStatus::Error;
            lastErrorMessage_ = "Cannot reach Spotify - check your internet";
        }
        else if(!deviceActive_){
            status_ = SpotifyStatus::NoActiveDevice;
        }
        else{
            status_ = SpotifyStatus::Connected;
        }
    }
}

// JSON Parsing =====================
void SpotifyClient::parsePlaybackState(const juce::var &json){
    const auto *obj = json.getDynamicObject();

    if(!obj)
        return;

    playing = obj->getProperty("is_playing");

    if(obj->hasProperty("item")){
        const auto item = obj->getProperty("item");
        currentTrack = item.getProperty("name", juce::var()).toString();
        currentAlbum = item.getProperty("album", juce::var()).getProperty("name", juce::var()).toString();
        const auto artists = item.getProperty("artists", juce::var());

        if(artists.isArray() && artists.size() > 0)
            currentArtist = artists[0].getProperty("name", juce::var()).toString();

        const auto images = item.getProperty("album", juce::var()).getProperty("images", juce::var());

        if(images.isArray()){
            const auto lastImage = images[images.size() - 1];

            if(lastImage.isObject())
                currentAlbumArtUrl = lastImage.getProperty("url", juce::var()).toString();
        }
    }
}

void SpotifyClient::parseDevices(const juce::var &json){
    const auto devices = json.getProperty("devices", juce::var());

    if(!devices.isArray())
        return;

    deviceActive_ = false;

    for(size_t i = 0; i < devices.size(); i++){
        const auto device = devices[i];

        if(device.getProperty("is_active", juce::var())){
            currentDeviceName = device.getProperty("name", juce::var()).toString();
            currentVolume = static_cast<int>(device.getProperty("volume_percent", juce::var()));
            deviceActive_ = true;
            break;
        }
    }

    if(!deviceActive_){
        currentDeviceName.clear();
        currentVolume = 0;
    }
}

// Others =====================

juce::String SpotifyClient::trackTitle() const { 
    const juce::ScopedLock sl(lock); 
    return currentTrack; 
}

juce::String SpotifyClient::trackArtist() const { 
    const juce::ScopedLock sl(lock); 
    return currentArtist; 
}

juce::String SpotifyClient::albumName() const { 
    const juce::ScopedLock sl(lock); 
    return currentAlbum; 
}

juce::String SpotifyClient::albumArtUrl() const { 
    const juce::ScopedLock sl(lock); 
    return currentAlbumArtUrl; 
}

juce::String SpotifyClient::deviceName() const { 
    const juce::ScopedLock sl(lock); 
    return currentDeviceName; 
}

bool SpotifyClient::isPlaying() const { 
    const juce::ScopedLock sl(lock); 
    return playing; 
}

int SpotifyClient::deviceVolume() const { 
    const juce::ScopedLock sl(lock); 
    return currentVolume; 
}

SpotifyStatus SpotifyClient::status() const {
    const juce::ScopedLock sl(lock);
    return status_;
}

juce::String SpotifyClient::lastErrorMessage() const {
    const juce::ScopedLock sl(lock);
    return lastErrorMessage_;
}

bool SpotifyClient::hasActiveDevice() const {
    const juce::ScopedLock sl(lock);
    return deviceActive_;
}