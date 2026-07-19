#include <juce_core/juce_core.h>

#include "core/AppSettings.h"
#include "core/SpotifyClient.h"

class SpotifyClientTests final : public juce::UnitTest {
public:
    SpotifyClientTests(): juce::UnitTest("SpotifyClient", "Phase 4"){}

    void runTest() override {
        beginTest("constructor — starts disconnected with no active device");
        {
            SpotifyClient client;

            expect(client.isAuthenticated() == false);
            expect(client.status() == SpotifyStatus::Disconnected);
            expect(client.hasActiveDevice() == false);
            expect(client.isPlaying() == false);
        }

        beginTest("disconnect — clears state and returns to Disconnected");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;
            client.loadTokens(settings);

            expect(client.isAuthenticated() == true);
            expect(client.status() == SpotifyStatus::Connected);

            client.disconnect();

            expect(client.isAuthenticated() == false);
            expect(client.status() == SpotifyStatus::Disconnected);
            expect(client.hasActiveDevice() == false);
            expect(client.isPlaying() == false);
        }

        beginTest("loadTokens — future expiry transitions to Connected");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;

            client.loadTokens(settings);

            expect(client.isAuthenticated() == true);
            expect(client.status() == SpotifyStatus::Connected);
        }

        beginTest("loadTokens — no saved tokens stays Disconnected");
        {
            SpotifyClient client;
            AppSettings settings;
            client.loadTokens(settings);

            expect(client.isAuthenticated() == false);
            expect(client.status() == SpotifyStatus::Disconnected);
        }

        beginTest("loadTokens — access token without refresh token stays Disconnected");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";

            client.loadTokens(settings);

            expect(client.isAuthenticated() == false);
            expect(client.status() == SpotifyStatus::Disconnected);
        }

        beginTest("loadTokens — zero expiry without refresh token stays Disconnected");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = 0;

            client.loadTokens(settings);

            expect(client.isAuthenticated() == false);
            expect(client.status() == SpotifyStatus::Disconnected);
        }

        beginTest("track and device accessors — return empty by default");
        {
            SpotifyClient client;

            expect(client.trackTitle().isEmpty());
            expect(client.trackArtist().isEmpty());
            expect(client.albumName().isEmpty());
            expect(client.albumArtUrl().isEmpty());
            expect(client.deviceName().isEmpty());
            expectEquals(client.deviceVolume(), 0);
        }

        beginTest("lastErrorMessage — empty after successful loadTokens");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;
            client.loadTokens(settings);

            expect(client.lastErrorMessage().isEmpty());
        }

        beginTest("lastErrorMessage — empty after disconnect");
        {
            SpotifyClient client;

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;
            client.loadTokens(settings);

            client.disconnect();

            expect(client.lastErrorMessage().isEmpty());
        }

        beginTest("hasActiveDevice — stays false after loadTokens without polling");
        {
            SpotifyClient client;

            expect(client.hasActiveDevice() == false);

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;
            client.loadTokens(settings);

            expect(client.hasActiveDevice() == false);

            client.disconnect();
            expect(client.hasActiveDevice() == false);
        }

        beginTest("isPlaying — returns false by default and after disconnect");
        {
            SpotifyClient client;

            expect(client.isPlaying() == false);

            AppSettings settings;
            settings.spotifyAccessToken = "test_access";
            settings.spotifyRefreshToken = "test_refresh";
            settings.spotifyTokenExpiry = juce::Time::getApproximateMillisecondCounter() + 60000;
            client.loadTokens(settings);

            expect(client.isPlaying() == false); 

            client.disconnect();
            expect(client.isPlaying() == false);
        }
    }
};

static SpotifyClientTests spotifyClientTests;
