/*
 * Minimal xerces-c stub for iZ3D build.
 * Provides enough symbols to link S3DWrapper9.
 * XML config parsing will be non-functional.
 *
 * All methods are defined OUT-OF-LINE so MSVC emits external symbols.
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdlib>
#include <cstring>

#pragma warning(disable: 4273) // inconsistent dll linkage

namespace xercesc_3_0 {

    // Forward declarations
    class DOMDocument;
    class XMLValidator;
    class MemoryManager;
    class XMLGrammarPool;
    class PanicHandler;
    class DOMNode;

    // ---- XMemory ----
    class XMemory {
    public:
        static void* operator new(unsigned int size);
        static void operator delete(void* p);
        virtual ~XMemory();
    };

    void* XMemory::operator new(unsigned int size) { return ::malloc(size); }
    void XMemory::operator delete(void* p) { ::free(p); }
    XMemory::~XMemory() {}

    // ---- MemoryManager ----
    class MemoryManager {
    public:
        virtual ~MemoryManager();
        virtual void* allocate(unsigned int size);
        virtual void deallocate(void* p);
    };

    MemoryManager::~MemoryManager() {}
    void* MemoryManager::allocate(unsigned int size) { return ::malloc(size); }
    void MemoryManager::deallocate(void* p) { ::free(p); }

    // Default memory manager instance
    static MemoryManager s_defaultMemoryManager;

    // ---- XMLPlatformUtils ----
    class XMLPlatformUtils {
    public:
        static MemoryManager* fgMemoryManager;
        static void Initialize(
            const char* const,
            const char* const,
            PanicHandler* const,
            MemoryManager* const);
        static void Terminate();
    };

    MemoryManager* XMLPlatformUtils::fgMemoryManager = &s_defaultMemoryManager;

    void XMLPlatformUtils::Initialize(
        const char* const,
        const char* const,
        PanicHandler* const,
        MemoryManager* const)
    {
        if (!fgMemoryManager)
            fgMemoryManager = &s_defaultMemoryManager;
    }

    void XMLPlatformUtils::Terminate() {
        // no-op
    }

    // ---- XMLUni ----
    class XMLUni {
    public:
        static const char* const fgXercescDefaultLocale;
    };
    const char* const XMLUni::fgXercescDefaultLocale = "en_US";

    // ---- AbstractDOMParser ----
    class AbstractDOMParser : public XMemory {
    public:
        virtual ~AbstractDOMParser();
        DOMDocument* getDocument();
        void parse(const wchar_t* const);
        void parse(const char* const);
    };

    AbstractDOMParser::~AbstractDOMParser() {}
    DOMDocument* AbstractDOMParser::getDocument() { return nullptr; }
    void AbstractDOMParser::parse(const wchar_t* const) { /* no-op */ }
    void AbstractDOMParser::parse(const char* const) { /* no-op */ }

    // ---- XercesDOMParser ----
    class XercesDOMParser : public AbstractDOMParser {
    public:
        XercesDOMParser(
            XMLValidator* const,
            MemoryManager* const,
            XMLGrammarPool* const);
        virtual ~XercesDOMParser();
    };

    XercesDOMParser::XercesDOMParser(
        XMLValidator* const,
        MemoryManager* const,
        XMLGrammarPool* const)
    {
        // no-op stub
    }
    XercesDOMParser::~XercesDOMParser() {}

    // ---- DOMNodeList stub ----
    class DOMNodeList : public XMemory {
    public:
        virtual ~DOMNodeList();
        virtual DOMNode* item(unsigned int) const;
        virtual unsigned int getLength() const;
    };

    DOMNodeList::~DOMNodeList() {}
    DOMNode* DOMNodeList::item(unsigned int) const { return nullptr; }
    unsigned int DOMNodeList::getLength() const { return 0; }

    // ---- DOMNode stub ----
    class DOMNode : public XMemory {
    public:
        virtual ~DOMNode();
    };
    DOMNode::~DOMNode() {}

    // ---- DOMDocument stub ----
    class DOMDocument : public DOMNode {
    public:
        virtual ~DOMDocument();
    };
    DOMDocument::~DOMDocument() {}

} // namespace xercesc_3_0
