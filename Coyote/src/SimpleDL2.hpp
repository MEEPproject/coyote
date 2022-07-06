
#pragma once

#include "sparta/utils/SpartaAssert.hpp"
#include "sparta/utils/MathUtils.hpp"
#include "cache/BasicCacheItem.hpp"
#include "cache_helpers/SimpleCache.hpp"
#include "cache/ReplacementIF.hpp"
#include "cache/preload/PreloadableIF.hpp"
#include "cache/preload/PreloadableNode.hpp"

using namespace std::placeholders;
namespace coyote
{
    class SimpleCacheLine : public sparta::cache::BasicCacheItem
    {
    public:
        SimpleCacheLine() = delete;

        SimpleCacheLine(uint64_t line_size) :
            line_size_(line_size),
            valid_(false),
            modified_(false),
            accessed_by_vector_(false),
            accessed_by_non_vector_(false)
        {
            sparta_assert(sparta::utils::is_power_of_2(line_size),
                "Cache line size must be a power of 2. line_size=" << line_size);
        }

        // Copy constructor
        SimpleCacheLine(const SimpleCacheLine & rhs) :
            BasicCacheItem(rhs),
            line_size_(rhs.line_size_),
            valid_(rhs.valid_),
            modified_(rhs.modified_),
            accessed_by_vector_(rhs.accessed_by_vector_),
            accessed_by_non_vector_(rhs.accessed_by_non_vector_)
        {
        }

        // Copy assignment operator
        SimpleCacheLine& operator=(const SimpleCacheLine & rhs)
        {
            BasicCacheItem::operator=(rhs);
            line_size_ = rhs.line_size_;
            valid_ = rhs.valid_;

            return *this;
        }

        virtual ~SimpleCacheLine() {}

        // Required by SimpleCache2
        void reset(uint64_t addr)
        {
            setValid(true);
            setModified(false);
            setAccessedByVector(false);
            setAccessedByNonVector(false);
            BasicCacheItem::setAddr(addr);
        }

        // Required by SimpleCache2
        void setValid(bool v) { valid_ = v; }

        // Required by BasicCacheSet
        bool isValid() const { return valid_; }

        // Required by SimpleCache2
        void setModified(bool m) { modified_=m; }
        bool isModified() { return modified_;}
        void setAccessedByVector(bool a) { accessed_by_vector_=a; }
        bool getAccessedByVector() { return accessed_by_vector_; }
        void setAccessedByNonVector(bool a) { accessed_by_non_vector_=a; }
        bool getAccessedByNonVector() { return accessed_by_non_vector_; }

        // Required by SimpleCache2
        bool read(uint64_t offset, uint32_t size, uint32_t *buf) const
        {
            (void) offset;
            (void) size;
            (void) buf;
            sparta_assert(false);
            return true;
        }

        // Required by SimpleCache2
        bool write(uint64_t offset, uint32_t size, uint32_t *buf) const
        {
            (void) offset;
            (void) size;
            (void) buf;
            sparta_assert(false);
            return true;
        }



        private:
        uint32_t line_size_;
        bool valid_;
        bool modified_;
        bool accessed_by_vector_;
        bool accessed_by_non_vector_;
        }; // class SimpleCacheLine

    //class SimpleDL2 : public sparta::cache::SimpleCache2<SimpleCacheLine>,
    class SimpleDL2 : public sparta::cache::SimpleCache<SimpleCacheLine>,
                      public sparta::TreeNode,
                      public sparta::cache::PreloadableIF,
                      public sparta::cache::PreloadDumpableIF

    {
    public:
        using Handle = std::shared_ptr<SimpleDL2>;
        SimpleDL2(sparta::TreeNode* parent,
                  uint64_t cache_size_kb,
                  uint64_t line_size,
                  uint64_t bank_and_tile_offset,
                  const sparta::cache::ReplacementIF& rep) :
            sparta::cache::SimpleCache<SimpleCacheLine> (cache_size_kb,
                                                        line_size,
                                                        bank_and_tile_offset,
                                                        SimpleCacheLine(line_size),
                                                        rep),
            sparta::TreeNode(parent, "l1cache", "Simple L1 Cache"),
            sparta::cache::PreloadableIF(),
            sparta::cache::PreloadDumpableIF(),
            preloadable_(this, std::bind(&SimpleDL2::preloadPkt_, this, _1),
                         std::bind(&SimpleDL2::preloadDump_, this, _1))
        {}
    private:
        /**
         * Implement a preload by just doing a fill to the va in the packet.
         */
        bool preloadPkt_(sparta::cache::PreloadPkt& pkt) override
        {
            sparta::cache::PreloadPkt::NodeList lines;
            pkt.getList(lines);
            for (auto& line_data : lines)
            {
                uint64_t va = line_data->getScalar<uint64_t>("va");
                auto& cache_line = getLineForReplacement(va);
                std::cout << *this << " : Preloading VA: 0x" << std::hex << va
                          << std::endl;
                allocateWithMRUUpdate(cache_line, va, false);
                // Sanity check that the line was marked as valid.
                sparta_assert(getLine(va) != nullptr);
            }
            return true;

        }

        void preloadDump_(sparta::cache::PreloadEmitter& emitter) const override
        {
            emitter << sparta::cache::PreloadEmitter::BeginMap;
            emitter << sparta::cache::PreloadEmitter::Key << "lines";
            emitter << sparta::cache::PreloadEmitter::Value;
            emitter << sparta::cache::PreloadEmitter::BeginSeq;
            for (auto set_it = begin();
                 set_it != end(); ++set_it)
            {
                for (auto line_it = set_it->begin();
                     line_it != set_it->end(); ++line_it)
                {
                    if(line_it->isValid())
                    {
                        std::map<std::string, std::string> map;
                        std::stringstream t;
                        t << "0x" << std::hex << line_it->getAddr();
                        map["pa"] = t.str();
                        emitter << map;
                    }
                }
            }
            emitter << sparta::cache::PreloadEmitter::EndSeq;
            emitter << sparta::cache::PreloadEmitter::EndMap;
        }
        //! Provide a preloadable node that hangs off and just returns
        //! the preloadPkt call to us.
        sparta::cache::PreloadableNode preloadable_;

    }; // class SimpleDL2

} // namespace core_example

