#include "dirty_rect_manager.h"
#include "logger.h"
#include <algorithm>

DirtyRectManager::DirtyRectManager() {
}

DirtyRectManager::~DirtyRectManager() {
}

void DirtyRectManager::Initialize(int screenWidth, int screenHeight, int tileSize) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_tileSize = tileSize;
    
    m_tilesX = (screenWidth + tileSize - 1) / tileSize;  // Ceiling division
    m_tilesY = (screenHeight + tileSize - 1) / tileSize;
    
    // Initialize dirty grid
    m_dirtyGrid.resize(m_tilesY);
    for (auto& row : m_dirtyGrid) {
        row.resize(m_tilesX, false);
    }
    
    // Reserve space for typical dirty region count
    m_dirtyRegions.reserve(m_tilesX * m_tilesY / 4); // Estimate 25% dirty
    m_dirtyTiles.reserve(m_tilesX * m_tilesY / 4);
    
    LOG_DEBUG("DirtyRectManager initialized: " + 
             std::to_string(m_tilesX) + "x" + std::to_string(m_tilesY) + 
             " tiles (" + std::to_string(tileSize) + "px each)");
}

void DirtyRectManager::Reset() {
    ClearDirtyFlags();
}

void DirtyRectManager::MarkDirty(const D2D1_RECT_F& rect) {
    if (!m_enabled) return;
    
    // Convert rect to tile coordinates
    int leftTile = std::max(0, GetTileX(rect.left));
    int topTile = std::max(0, GetTileY(rect.top));
    int rightTile = std::min(m_tilesX - 1, GetTileX(rect.right));
    int bottomTile = std::min(m_tilesY - 1, GetTileY(rect.bottom));
    
    // Mark all overlapping tiles as dirty
    for (int y = topTile; y <= bottomTile; ++y) {
        for (int x = leftTile; x <= rightTile; ++x) {
            if (!m_dirtyGrid[y][x]) {
                m_dirtyGrid[y][x] = true;
                m_dirtyTiles.insert(GetTileIndex(x, y));
                m_regionsNeedUpdate = true;
            }
        }
    }
}

void DirtyRectManager::MarkDirty(int gridX, int gridY) {
    if (!m_enabled || gridX < 0 || gridY < 0 || gridX >= m_tilesX || gridY >= m_tilesY) {
        return;
    }
    
    if (!m_dirtyGrid[gridY][gridX]) {
        m_dirtyGrid[gridY][gridX] = true;
        m_dirtyTiles.insert(GetTileIndex(gridX, gridY));
        m_regionsNeedUpdate = true;
    }
}

void DirtyRectManager::MarkDirty(int gridX, int gridY, int width, int height) {
    if (!m_enabled) return;
    
    // Clamp to valid range
    int endX = std::min(gridX + width, m_tilesX);
    int endY = std::min(gridY + height, m_tilesY);
    gridX = std::max(0, gridX);
    gridY = std::max(0, gridY);
    
    for (int y = gridY; y < endY; ++y) {
        for (int x = gridX; x < endX; ++x) {
            if (!m_dirtyGrid[y][x]) {
                m_dirtyGrid[y][x] = true;
                m_dirtyTiles.insert(GetTileIndex(x, y));
            }
        }
    }
    
    if (endX > gridX && endY > gridY) {
        m_regionsNeedUpdate = true;
    }
}

bool DirtyRectManager::IsRegionDirty(int tileX, int tileY) const {
    if (!m_enabled || tileX < 0 || tileY < 0 || tileX >= m_tilesX || tileY >= m_tilesY) {
        return true; // Assume dirty if out of bounds
    }
    
    return m_dirtyGrid[tileY][tileX];
}

bool DirtyRectManager::IsRectDirty(const D2D1_RECT_F& rect) const {
    if (!m_enabled) return true;
    
    // Check if any tile overlapping this rect is dirty
    int leftTile = std::max(0, GetTileX(rect.left));
    int topTile = std::max(0, GetTileY(rect.top));
    int rightTile = std::min(m_tilesX - 1, GetTileX(rect.right));
    int bottomTile = std::min(m_tilesY - 1, GetTileY(rect.bottom));
    
    for (int y = topTile; y <= bottomTile; ++y) {
        for (int x = leftTile; x <= rightTile; ++x) {
            if (m_dirtyGrid[y][x]) {
                return true;
            }
        }
    }
    
    return false;
}

void DirtyRectManager::ClearDirtyFlags() {
    if (!m_enabled) return;
    
    // Reset all dirty flags
    for (auto& row : m_dirtyGrid) {
        std::fill(row.begin(), row.end(), false);
    }
    
    m_dirtyTiles.clear();
    m_dirtyRegions.clear();
    m_regionsNeedUpdate = false;
}

void DirtyRectManager::MarkFullScreenDirty() {
    if (!m_enabled) return;
    
    // Mark all tiles as dirty
    for (int y = 0; y < m_tilesY; ++y) {
        for (int x = 0; x < m_tilesX; ++x) {
            m_dirtyGrid[y][x] = true;
            m_dirtyTiles.insert(GetTileIndex(x, y));
        }
    }
    
    m_regionsNeedUpdate = true;
}

float DirtyRectManager::GetDirtyPercentage() const {
    if (!m_enabled || m_tilesX * m_tilesY == 0) return 0.0f;
    
    return (static_cast<float>(m_dirtyTiles.size()) / (m_tilesX * m_tilesY)) * 100.0f;
}

void DirtyRectManager::UpdateDirtyRegions() {
    if (!m_regionsNeedUpdate) return;
    
    m_dirtyRegions.clear();
    
    // Convert dirty tiles to rectangles
    for (int tileIndex : m_dirtyTiles) {
        int tileX = tileIndex % m_tilesX;
        int tileY = tileIndex / m_tilesX;
        
        DirtyRegion region;
        region.rect = TileToRect(tileX, tileY);
        region.needsRedraw = true;
        
        m_dirtyRegions.push_back(region);
    }
    
    // Optimize by merging adjacent rectangles
    OptimizeRegions();
    
    m_regionsNeedUpdate = false;
}

D2D1_RECT_F DirtyRectManager::TileToRect(int tileX, int tileY) const {
    float left = static_cast<float>(tileX * m_tileSize);
    float top = static_cast<float>(tileY * m_tileSize);
    float right = std::min(left + m_tileSize, static_cast<float>(m_screenWidth));
    float bottom = std::min(top + m_tileSize, static_cast<float>(m_screenHeight));
    
    return D2D1_RECT_F{left, top, right, bottom};
}

void DirtyRectManager::OptimizeRegions() {
    // Simple optimization: merge horizontally adjacent rectangles
    // More sophisticated algorithms could merge in 2D
    
    if (m_dirtyRegions.size() < 2) return;
    
    std::vector<DirtyRegion> optimized;
    optimized.reserve(m_dirtyRegions.size());
    
    // Sort by top, then left
    std::sort(m_dirtyRegions.begin(), m_dirtyRegions.end(),
        [](const DirtyRegion& a, const DirtyRegion& b) {
            if (a.rect.top != b.rect.top) return a.rect.top < b.rect.top;
            return a.rect.left < b.rect.left;
        });
    
    DirtyRegion current = m_dirtyRegions[0];
    
    for (size_t i = 1; i < m_dirtyRegions.size(); ++i) {
        const auto& next = m_dirtyRegions[i];
        
        // Check if we can merge horizontally
        if (current.rect.top == next.rect.top &&
            current.rect.bottom == next.rect.bottom &&
            current.rect.right == next.rect.left) {
            
            // Merge rectangles
            current.rect.right = next.rect.right;
        } else {
            // Can't merge, save current and start new one
            optimized.push_back(current);
            current = next;
        }
    }
    
    optimized.push_back(current);
    m_dirtyRegions = std::move(optimized);
}