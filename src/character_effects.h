#pragma once

#include "common.h"

class CharacterEffects {
public:
    CharacterEffects();
    ~CharacterEffects();
    
    void Initialize(const MatrixSettings& settings);
    void Update(float deltaTime);
    void SetSettings(const MatrixSettings& settings);
    
    // Character selection with variety
    std::wstring SelectCharacter(float depth = 0.5f, bool allowVariety = true) const;
    std::wstring SelectMorphTarget(const std::wstring& current) const;
    
    // Morphing system
    void StartMorphing(GridCell& cell, float probability);
    void UpdateMorphing(GridCell& cell, float deltaTime);
    std::wstring GetMorphedCharacter(const GridCell& cell) const;
    
    // Glitch effects
    void StartGlitch(GridCell& cell, float probability);
    void UpdateGlitch(GridCell& cell, float deltaTime);
    std::wstring GetGlitchedCharacter(const GridCell& cell) const;
    
    // Phosphor glow effects
    void UpdateGlow(GridCell& cell, float deltaTime);
    Color GetGlowColor(const GridCell& cell) const;
    
    // System-wide effects
    void TriggerSystemDisruption();
    bool IsSystemDisrupted() const { return m_systemDisruptionTimer > 0.0f; }
    float GetSystemDisruptionIntensity() const;
    
    // Rain variation effects
    float GetRainIntensityMultiplier() const;
    void UpdateRainVariations(float deltaTime);

private:
    MatrixSettings m_settings;
    
    // System disruption
    float m_systemDisruptionTimer = 0.0f;
    float m_systemDisruptionDuration = 2.0f;
    float m_timeSinceLastDisruption = 0.0f;
    
    // Rain variations
    float m_rainIntensityPhase = 0.0f;
    float m_baseRainIntensity = 1.0f;
    
    // Character pools for efficiency
    mutable std::vector<std::wstring> m_availableChars;
    mutable std::vector<std::wstring> m_morphTargets;
    
    // Helper methods
    void RebuildCharacterPools();
    std::wstring SelectFromPool(const std::vector<std::wstring>& pool) const;
    float GetCharacterWeight(const std::wstring& character, float depth) const;
    
    // Morphing interpolation
    std::wstring InterpolateCharacters(const std::wstring& from, const std::wstring& to, float progress) const;
};