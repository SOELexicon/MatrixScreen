#pragma once

#include <vector>
#include <stack>
#include <memory>
#include <mutex>

template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t initialSize = 1000, size_t growthSize = 500)
        : m_growthSize(growthSize) {
        ExpandPool(initialSize);
    }
    
    ~MemoryPool() {
        // All objects should be returned before destruction
    }
    
    // Get an object from the pool
    T* Acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_available.empty()) {
            ExpandPool(m_growthSize);
        }
        
        T* obj = m_available.top();
        m_available.pop();
        
        // Reset object to default state
        *obj = T{};
        
        return obj;
    }
    
    // Return an object to the pool
    void Release(T* obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available.push(obj);
    }
    
    size_t GetAvailableCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_available.size();
    }
    
    size_t GetTotalCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_storage.size();
    }

private:
    void ExpandPool(size_t count) {
        size_t currentSize = m_storage.size();
        m_storage.resize(currentSize + count);
        
        // Add new objects to available stack
        for (size_t i = currentSize; i < m_storage.size(); ++i) {
            m_available.push(&m_storage[i]);
        }
    }
    
    mutable std::mutex m_mutex;
    std::vector<T> m_storage;           // Actual object storage
    std::stack<T*> m_available;        // Available objects
    size_t m_growthSize;               // How much to grow when pool is empty
};

// RAII wrapper for automatic memory pool management
template<typename T>
class PooledObject {
public:
    PooledObject(MemoryPool<T>* pool) : m_pool(pool), m_object(pool->Acquire()) {}
    
    ~PooledObject() {
        if (m_object && m_pool) {
            m_pool->Release(m_object);
        }
    }
    
    // Non-copyable but movable
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;
    
    PooledObject(PooledObject&& other) noexcept 
        : m_pool(other.m_pool), m_object(other.m_object) {
        other.m_pool = nullptr;
        other.m_object = nullptr;
    }
    
    PooledObject& operator=(PooledObject&& other) noexcept {
        if (this != &other) {
            if (m_object && m_pool) {
                m_pool->Release(m_object);
            }
            
            m_pool = other.m_pool;
            m_object = other.m_object;
            
            other.m_pool = nullptr;
            other.m_object = nullptr;
        }
        return *this;
    }
    
    T* Get() const { return m_object; }
    T* operator->() const { return m_object; }
    T& operator*() const { return *m_object; }
    
    explicit operator bool() const { return m_object != nullptr; }

private:
    MemoryPool<T>* m_pool;
    T* m_object;
};