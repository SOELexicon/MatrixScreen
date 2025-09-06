#include "character_effects.h"
#include "logger.h"
#include <algorithm>
#include <cmath>

CharacterEffects::CharacterEffects() {
}

CharacterEffects::~CharacterEffects() {
}

void CharacterEffects::Initialize(const MatrixSettings& settings) {
    m_settings = settings;
    RebuildCharacterPools();
    
    LOG_DEBUG("CharacterEffects initialized with variety: " + 
             std::to_string(settings.enableCharacterVariety));
}

void CharacterEffects::Update(float deltaTime) {
    // Update system disruptions
    if (m_systemDisruptionTimer > 0.0f) {
        m_systemDisruptionTimer -= deltaTime;
    }
    
    m_timeSinceLastDisruption += deltaTime;
    
    // Randomly trigger system disruptions
    if (m_settings.enableSystemDisruptions && m_timeSinceLastDisruption > 30.0f) {
        float disruptionChance = deltaTime * 0.01f; // 1% chance per second after 30s
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng) < disruptionChance) {
            TriggerSystemDisruption();
        }
    }
    
    // Update rain variations
    if (m_settings.enableRainVariations) {
        UpdateRainVariations(deltaTime);
    }
}

void CharacterEffects::SetSettings(const MatrixSettings& settings) {
    m_settings = settings;
    RebuildCharacterPools();
}

std::wstring CharacterEffects::SelectCharacter(float depth, bool allowVariety) const {
    if (!allowVariety || !m_settings.enableCharacterVariety || m_availableChars.empty()) {
        // Use original character set
        return KATAKANA_CHARS[std::uniform_int_distribution<int>(0, static_cast<int>(KATAKANA_CHARS.size()) - 1)(g_rng)];
    }
    
    // Weighted character selection based on depth and probability settings
    float roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng);
    
    // Symbols are rarer in deeper areas (darker mask areas)
    float adjustedSymbolProb = m_settings.symbolCharProbability * (1.0f - depth * 0.5f);
    float adjustedLatinProb = m_settings.latinCharProbability;
    
    if (roll < adjustedSymbolProb && !SYMBOL_CHARS.empty()) {
        return SYMBOL_CHARS[std::uniform_int_distribution<int>(0, static_cast<int>(SYMBOL_CHARS.size()) - 1)(g_rng)];
    } else if (roll < adjustedSymbolProb + adjustedLatinProb && !LATIN_CHARS.empty()) {
        return LATIN_CHARS[std::uniform_int_distribution<int>(0, static_cast<int>(LATIN_CHARS.size()) - 1)(g_rng)];
    } else {
        // Default to Japanese characters
        return KATAKANA_CHARS[std::uniform_int_distribution<int>(0, static_cast<int>(KATAKANA_CHARS.size()) - 1)(g_rng)];
    }
}

std::wstring CharacterEffects::SelectMorphTarget(const std::wstring& current) const {
    if (m_morphTargets.empty()) {
        return SelectCharacter();
    }
    
    // Select a different character for morphing
    std::wstring target;
    int attempts = 0;
    do {
        target = SelectFromPool(m_morphTargets);
        attempts++;
    } while (target == current && attempts < 10);
    
    return target;
}

void CharacterEffects::StartMorphing(GridCell& cell, float probability) {
    if (!m_settings.enableCharacterMorphing) return;
    
    float roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng);
    if (roll < probability && !cell.isMorphing) {
        cell.morphTarget = SelectMorphTarget(cell.character);
        cell.morphProgress = 0.0f;
        cell.morphSpeed = m_settings.morphSpeed * std::uniform_real_distribution<float>(0.8f, 1.2f)(g_rng);
        cell.morphTimer = 0.0f;
        cell.isMorphing = true;
    }
}

void CharacterEffects::UpdateMorphing(GridCell& cell, float deltaTime) {
    if (!cell.isMorphing) return;
    
    cell.morphTimer += deltaTime;
    cell.morphProgress += deltaTime * cell.morphSpeed;
    
    if (cell.morphProgress >= 1.0f) {
        // Morphing complete
        cell.character = cell.morphTarget;
        cell.morphTarget = L"";
        cell.morphProgress = 0.0f;
        cell.isMorphing = false;
        
        // Chance to start another morph
        if (std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng) < 0.3f) {
            StartMorphing(cell, 1.0f); // 100% chance for chain morphing
        }
    }
}

std::wstring CharacterEffects::GetMorphedCharacter(const GridCell& cell) const {
    if (!cell.isMorphing || cell.morphTarget.empty()) {
        return cell.character;
    }
    
    // Simple character switching based on progress
    if (cell.morphProgress < 0.5f) {
        return cell.character;
    } else {
        return cell.morphTarget;
    }
}

void CharacterEffects::StartGlitch(GridCell& cell, float probability) {
    if (!m_settings.enableGlitchEffects) return;
    
    float roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng);
    if (roll < probability && !cell.isGlitching) {
        cell.glitchIntensity = std::uniform_real_distribution<float>(0.5f, 1.0f)(g_rng);
        cell.glitchTimer = 0.0f;
        cell.isGlitching = true;
    }
}

void CharacterEffects::UpdateGlitch(GridCell& cell, float deltaTime) {
    if (!cell.isGlitching) return;
    
    cell.glitchTimer += deltaTime;
    
    // Glitch lasts 0.1 to 0.3 seconds
    float glitchDuration = 0.1f + cell.glitchIntensity * 0.2f;
    if (cell.glitchTimer >= glitchDuration) {
        cell.isGlitching = false;
        cell.glitchIntensity = 0.0f;
        cell.glitchTimer = 0.0f;
    }
}

std::wstring CharacterEffects::GetGlitchedCharacter(const GridCell& cell) const {
    if (!cell.isGlitching) {
        return GetMorphedCharacter(cell);
    }
    
    // During glitch, rapidly switch between random characters
    if (static_cast<int>(cell.glitchTimer * 20.0f) % 2 == 0) {
        return SelectCharacter(cell.depth, m_settings.enableCharacterVariety);
    } else {
        return GetMorphedCharacter(cell);
    }
}

void CharacterEffects::UpdateGlow(GridCell& cell, float deltaTime) {
    if (!m_settings.enablePhosphorGlow) {
        cell.glowIntensity = 0.0f;
        return;
    }
    
    // Vary glow intensity based on character activity and alpha
    float targetGlow = cell.alpha * m_settings.glowIntensity;
    
    // Add some variation for more organic feel
    targetGlow += std::sin(cell.lastUpdateTime * 3.0f) * 0.1f * m_settings.glowIntensity;
    targetGlow = std::max(0.0f, targetGlow);
    
    // Smooth transition to target glow
    cell.glowIntensity += (targetGlow - cell.glowIntensity) * deltaTime * 5.0f;
    
    // Set glow color based on character properties
    if (cell.glowIntensity > 0.0f) {
        Color baseColor = Color(0.0f, 1.0f, 0.0f, cell.glowIntensity);
        
        // Modify color based on character type
        if (cell.isGlitching) {
            baseColor.r = 0.2f; // Slight red tint for glitches
        } else if (cell.isMorphing) {
            baseColor.b = 0.1f; // Slight blue tint for morphing
        }
        
        cell.glowColor = baseColor;
    }
}

Color CharacterEffects::GetGlowColor(const GridCell& cell) const {
    return cell.glowColor;
}

void CharacterEffects::TriggerSystemDisruption() {
    m_systemDisruptionTimer = m_systemDisruptionDuration;
    m_timeSinceLastDisruption = 0.0f;
    
    LOG_DEBUG("System disruption triggered");
}

float CharacterEffects::GetSystemDisruptionIntensity() const {
    if (m_systemDisruptionTimer <= 0.0f) return 0.0f;
    
    // Intensity peaks at start and fades out
    float progress = 1.0f - (m_systemDisruptionTimer / m_systemDisruptionDuration);
    return std::exp(-progress * 3.0f); // Exponential fade
}

float CharacterEffects::GetRainIntensityMultiplier() const {
    if (!m_settings.enableRainVariations) return 1.0f;
    
    // Combine slow wave with some randomness
    float slowWave = 0.8f + 0.4f * std::sin(m_rainIntensityPhase * 0.1f);
    float fastVariation = 0.9f + 0.2f * std::sin(m_rainIntensityPhase * 0.5f);
    
    return slowWave * fastVariation;
}

void CharacterEffects::UpdateRainVariations(float deltaTime) {
    m_rainIntensityPhase += deltaTime;
}

void CharacterEffects::RebuildCharacterPools() {
    m_availableChars.clear();
    m_morphTargets.clear();
    
    if (m_settings.enableCharacterVariety) {
        // Add all character types to pools
        m_availableChars.insert(m_availableChars.end(), MATRIX_CHARS.begin(), MATRIX_CHARS.end());
        m_morphTargets.insert(m_morphTargets.end(), MATRIX_CHARS.begin(), MATRIX_CHARS.end());
    } else {
        // Use only basic katakana
        m_availableChars.insert(m_availableChars.end(), KATAKANA_CHARS.begin(), KATAKANA_CHARS.end());
        m_morphTargets.insert(m_morphTargets.end(), KATAKANA_CHARS.begin(), KATAKANA_CHARS.end());
    }
}

std::wstring CharacterEffects::SelectFromPool(const std::vector<std::wstring>& pool) const {
    if (pool.empty()) return L"ア";
    
    int index = std::uniform_int_distribution<int>(0, static_cast<int>(pool.size()) - 1)(g_rng);
    return pool[index];
}

float CharacterEffects::GetCharacterWeight(const std::wstring& character, float depth) const {
    // Weight characters based on depth and type
    // Symbols are rarer in deep areas, Latin chars are consistent
    
    for (const auto& symbol : SYMBOL_CHARS) {
        if (character == symbol) {
            return 1.0f - depth * 0.7f; // Much rarer in deep areas
        }
    }
    
    for (const auto& latin : LATIN_CHARS) {
        if (character == latin) {
            return 0.8f; // Consistent weight
        }
    }
    
    // Default katakana weight
    return 1.0f;
}

std::wstring CharacterEffects::InterpolateCharacters(const std::wstring& from, const std::wstring& to, float progress) const {
    // Simple character switching - could be enhanced with visual morphing
    return progress < 0.5f ? from : to;
}