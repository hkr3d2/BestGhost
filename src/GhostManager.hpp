#pragma once
#include "GhostData.hpp"
#include <map>
#include <functional>

class GhostManager {
private:
    std::map<float, GhostData> m_ghosts;
    std::string m_currentLevelID;
    GhostData* m_activeGhost = nullptr;
    bool m_isRecording = false;
    bool m_isRacing = false;
    std::vector<GhostFrame> m_currentRecording;
    float m_recordingStartTime = 0;
    
    // Callbacks for UI updates
    std::function<void()> m_onGhostsUpdated;
    
    // Get folder path for current level
    std::string getLevelFolderPath();
    
    // Load all ghosts for current level
    void loadGhostsForCurrentLevel();
    
public:
    static GhostManager& get();
    
    // Set current level (call when entering a level)
    void setCurrentLevel(const std::string& levelID);
    
    // Get all ghosts (sorted by percentage)
    const std::map<float, GhostData>& getGhosts() const { return m_ghosts; }
    
    // Start recording a new attempt
    void startRecording();
    
    // Add a frame to current recording
    void recordFrame(float x, float y, float rotation, bool isHolding, int gameMode, float time);
    
    // Stop recording and check if this was a new best
    void stopRecording(float finalPercentage);
    
    // Set active ghost for racing
    void setActiveGhost(float percentage);
    
    // Get active ghost
    GhostData* getActiveGhost() { return m_activeGhost; }
    
    // Is racing mode enabled?
    bool isRacing() const { return m_isRacing; }
    
    // Enable/disable racing
    void setRacing(bool enabled);
    
    // Delete a ghost
    void deleteGhost(float percentage);
    
    // Delete all ghosts for current level
    void deleteAllGhosts();
    
    // Set callback for UI updates
    void setOnGhostsUpdatedCallback(std::function<void()> callback);
    
    // Export ghost to file
    bool exportGhost(float percentage, const std::string& path);
    
    // Import ghost from file
    bool importGhost(const std::string& path);
};
