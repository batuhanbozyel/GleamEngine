#pragma once

namespace Gleam {

class BinarySerializer final
{
public:
    
    BinarySerializer(std::iostream& stream)
        : mStream(stream)
    {
        
    }
    
    template<typename T>
    void Serialize(const TArray<T>& object)
    {
        mStream.write(reinterpret_cast<const char*>(object.data()), sizeof(T) * object.size());
    }
    
    template<typename K, typename V>
    void Serialize(const HashMap<K, V>& object)
    {
        for (const auto& [key, value] : object)
        {
            mStream.write(reinterpret_cast<const char*>(&key), sizeof(K));
            mStream.write(reinterpret_cast<const char*>(&value), sizeof(V));
        }
    }
    
    template<typename T>
    TArray<T> Deserialize()
    {
        mStream.seekg(0, std::ios::end);
        size_t size = mStream.tellg();
        mStream.seekg(0, std::ios::beg);
        
        TArray<T> object(size / sizeof(T));
        for (auto& element : object)
        {
            mStream.read(reinterpret_cast<char*>(&element), sizeof(T));
        }
        return object;
    }
    
    template<typename K, typename V>
    HashMap<K, V> Deserialize()
    {
        mStream.seekg(0, std::ios::end);
        size_t size = mStream.tellg();
        mStream.seekg(0, std::ios::beg);
        
        HashMap<K, V> object(size / (sizeof(K) + sizeof(V)));
        for (auto& [key, value] : object)
        {
            mStream.read(reinterpret_cast<char*>(&key), sizeof(K));
            mStream.read(reinterpret_cast<char*>(&value), sizeof(V));
        }
        return object;
    }
    
private:
    
    std::iostream& mStream;
    
};

} // namespace Gleam
