#pragma once

#include "common.h"
#include <vector>
#include <unordered_set>

struct DirtyRegion {
    D2D1_RECT_F rect;
    bool needsRedraw = false;
    
    DirtyRegion() : rect{0, 0, 0, 0} {}
    DirtyRegion(float left, float top, float right, float bottom) 
        : rect{left, top, right, bottom}, needsRedraw(true) {}
};

class DirtyRectManager {
public:
    DirtyRectManager();
    ~DirtyRectManager();
    
    void Initialize(int screenWidth, int screenHeight, int tileSize = 64);
    void Reset();
    
    // Mark a rectangular area as dirty
    void MarkDirty(const D2D1_RECT_F& rect);
    void MarkDirty(int gridX, int gridY);
    void MarkDirty(int gridX, int gridY, int width, int height);
    
    // Get all dirty rectangles that need redrawing
    const std::vector<DirtyRegion>& GetDirtyRegions() const { return m_dirtyRegions; }
    
    // Check if a specific region needs redrawing
    bool IsRegionDirty(int tileX, int tileY) const;
    bool IsRectDirty(const D2D1_RECT_F& rect) const;
    
    // Clear all dirty flags
    void ClearDirtyFlags();
    
    // Force full screen redraw
    void MarkFullScreenDirty();
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    
    // Debug information
    size_t GetDirtyTileCount() const { return m_dirtyTiles.size(); }
    float GetDirtyPercentage() const;

private:
    bool m_enabled = false;
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    int m_tileSize = 64;
    int m_tilesX = 0;
    int m_tilesY = 0;
    
    // Grid of dirty flags
    std::vector<std::vector<bool>> m_dirtyGrid;
    
    // Set of dirty tile indices for fast lookup
    std::unordered_set<int> m_dirtyTiles;
    
    // Cached dirty regions for rendering
    std::vector<DirtyRegion> m_dirtyRegions;
    bool m_regionsNeedUpdate = false;
    
    // Convert coordinates
    int GetTileX(float x) const { return static_cast<int>(x / m_tileSize); }
    int GetTileY(float y) const { return static_cast<int>(y / m_tileSize); }
    int GetTileIndex(int tileX, int tileY) const { return tileY * m_tilesX + tileX; }
    
    void UpdateDirtyRegions();
    D2D1_RECT_F TileToRect(int tileX, int tileY) const;
    void OptimizeRegions(); // Merge adjacent dirty rectangles
};