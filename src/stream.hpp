#ifndef HTCW_STREAM_HPP
#define HTCW_STREAM_HPP
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
namespace io {
    struct stream_caps {
        uint8_t read : 1;
        uint8_t write : 1;
        uint8_t seek :1;
    };
    enum struct seek_origin {
        start = 0,
        current = 1,
        end = 2
    };
    class stream {
    public:
        virtual int getc()=0;
        virtual size_t read(uint8_t* destination,size_t size)=0;
        virtual int putc(int value)=0;
        virtual size_t write(const uint8_t* source,size_t size)=0;
        virtual unsigned long long seek(long long position,seek_origin origin=seek_origin::start)=0;
        virtual stream_caps caps() const=0;
        template<typename T>
        size_t read(T* out_value) {
            return read((uint8_t*)out_value,sizeof(T));
        }
        template<typename T>
        T read(const T& default_value=T()) {
            T result;
            if(sizeof(T)!=read(&result))
                return default_value;
            return result;
        }
        template<typename T>
        size_t write(const T& value) {
            return write((uint8_t*)&value,sizeof(T));
        }
    };
    enum struct file_mode {
        read = 1,
        write = 2,
        append = 6
    };
    class buffer_stream final : public stream {
        uint8_t* m_begin;
        uint8_t* m_current;
        size_t m_size;
        buffer_stream(const buffer_stream& rhs)=delete;
        buffer_stream& operator=(const buffer_stream& rhs)=delete;
    public:
        buffer_stream() : m_begin(nullptr),m_current(nullptr),m_size(0) {}
        buffer_stream(uint8_t* buffer,size_t size) :m_begin(buffer),m_current(buffer),m_size(size) {
            if(nullptr==buffer) m_size=0;
        }
        buffer_stream(buffer_stream&& rhs) : m_begin(rhs.m_begin), m_current(rhs.m_current), m_size(rhs.m_size) {
            rhs.m_begin=rhs.m_current=nullptr;
            rhs.m_size=0;
        }
        buffer_stream& operator=(buffer_stream&& rhs) {
            m_begin=rhs.m_begin;
            m_current=rhs.m_current;
            m_size=rhs.m_size;
            rhs.m_begin=rhs.m_current=nullptr;
            rhs.m_size=0;
            return *this;
        }
        void set(uint8_t* buffer,size_t size) {
            m_begin = m_current = buffer;
            m_size = size;
            if(nullptr==buffer) m_size=0;
        }
        virtual int getc() {
            if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
                return -1;
            return *(m_current++);
        }
        virtual int putc(int value) {
            if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
                return -1;
            return *(m_current++)=(uint8_t)value;
        }
        virtual stream_caps caps() const {
            stream_caps s;
            s.read = s.write = s.seek = (nullptr!=m_current);
            return s;
        }
        virtual size_t read(uint8_t* destination,size_t size) {
            if(nullptr==m_current || nullptr==destination)
                return 0;
            size_t offs=m_current-m_begin;
            if(offs>=m_size)
                return 0;
            if(size+offs>m_size) 
                size = (m_size-offs);
            memcpy(destination,m_current,size);
            m_current+=size;
            return size;
        }
        
        virtual size_t write(const uint8_t* source,size_t size) {
            if(nullptr==m_current || nullptr==source)
                return 0;
            size_t offs=m_current-m_begin;
            if(offs>=m_size)
                return 0;
            if(size+offs>=m_size) 
                size = (m_size-offs);
            if(0!=size) {
                memcpy(m_current,source,size);
                m_current+=size;
            }
            return size;
        }
        
        virtual unsigned long long seek(long long position,seek_origin origin=seek_origin::start)  {
            if(nullptr==m_current)
                return 0;
            size_t offs=m_current-m_begin;
            size_t p;
            switch(origin) {
                case seek_origin::start:
                    if(position<0)
                        p = 0;
                    else if(0<=position && ((unsigned long long)position)>m_size) {
                        p=m_size;
                    } else
                        p=(size_t)position;
                    break;
                case seek_origin::current:
                    if(offs+position<0)
                        p = offs;
                    else if(offs+position>m_size) {
                        p=m_size;
                    } else
                        p=(size_t)(offs+position);
                    break;
                case seek_origin::end:
                    return seek(m_size-position,seek_origin::start);
                default:
                    return offs;
            }
            m_current=m_begin+p;
            return m_current-m_begin;   
        }
    };
    class file_stream final : public stream {
        FILE* m_fd;
        stream_caps m_caps;
        file_stream(const file_stream& rhs) = delete;
        file_stream& operator=(const file_stream& rhs)=delete;
    public:
        file_stream(const char* name,file_mode mode=file_mode::read) : m_caps({0,0,0}) {
            if((int)file_mode::append==((int)mode & (int)file_mode::append)) {
                if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                    m_fd = fopen(name,"a+");
                    if(nullptr!=m_fd) {
                        m_caps.read=1;
                        m_caps.write=1;
                        m_caps.seek=1;
                    }
                    return;
                }
                m_fd = fopen(name,"a");
                if(nullptr!=m_fd) {
                    m_caps.read=0;
                    m_caps.write=1;
                    m_caps.seek=1;
                }
                return;
            }
            if((int)file_mode::write==((int)mode & (int)file_mode::write)) {
                if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                    m_fd = fopen(name,"w+");
                    if(nullptr!=m_fd) {
                        m_caps.read=1;
                        m_caps.write=1;
                        m_caps.seek=1;
                    }
                    return;
                }
                m_fd = fopen(name,"w");
                if(nullptr!=m_fd) {
                    m_caps.read=0;
                    m_caps.write=1;
                    m_caps.seek=1;
                }
                return;
            }
            if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                m_fd = fopen(name,"r");
                if(nullptr!=m_fd) {
                    m_caps.read=1;
                    m_caps.write=0;
                    m_caps.seek=1;
                }
            }
        }
        file_stream(file_stream&& rhs) : m_fd(rhs.m_fd),m_caps(rhs.m_caps) {
            rhs.m_fd=nullptr;
            rhs.m_caps={0,0,0};
        }
        file_stream& operator=(file_stream&& rhs) {
            m_fd=rhs.m_fd;
            m_caps=rhs.m_caps;
            rhs.m_fd=nullptr;
            rhs.m_caps={0,0,0};
            return *this;
        }
        ~file_stream() {
            if(nullptr!=m_fd)
                fclose(m_fd);
        }
        
        virtual size_t read(uint8_t* destination,size_t size) {
            if(0==m_caps.read) return 0;
            return fread(destination,1,size,m_fd);
        }
        virtual int getc() {
            if(0==m_caps.read) return -1;
            return fgetc(m_fd);
        }
        virtual size_t write(const uint8_t* source,size_t size) {
            if(0==m_caps.write) return 0;
            return fwrite(source,1,size,m_fd);
        }
        virtual int putc(int value) {
            if(0==m_caps.write) return -1;
            return fputc(value,m_fd);
        } 
        virtual unsigned long long seek(long long position, seek_origin origin=seek_origin::start) {
            if(0==m_caps.seek) return 0; // should always be true unless uninitialized
            // on error we still need to report the current position
            // so we continue regardless
            if(position!=0||origin!=seek_origin::current)
                fseek(m_fd,(long int)position,(int)origin);
            return ftell(m_fd);
        }
        virtual stream_caps caps() const {
            return m_caps;
        }
    };
}
#endif