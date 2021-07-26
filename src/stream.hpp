#ifndef HTCW_STREAM_HPP
#define HTCW_STREAM_HPP
#include <bits.hpp>
#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
// TODO: Fix this ifdef so the AVR is the one that is special cased:
#ifdef ESP32
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#else
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#endif
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
    class const_buffer_stream final : public stream {
        const uint8_t* m_begin;
        const uint8_t* m_current;
        size_t m_size;
        const_buffer_stream(const const_buffer_stream& rhs)=delete;
        const_buffer_stream& operator=(const const_buffer_stream& rhs)=delete;
    public:
        const_buffer_stream() : m_begin(nullptr),m_current(nullptr),m_size(0) {}
        const_buffer_stream(const uint8_t* buffer,size_t size) :m_begin(buffer),m_current(buffer),m_size(size) {
            if(nullptr==buffer) m_size=0;
        }
        const_buffer_stream(const_buffer_stream&& rhs) : m_begin(rhs.m_begin), m_current(rhs.m_current), m_size(rhs.m_size) {
            rhs.m_begin=rhs.m_current=nullptr;
            rhs.m_size=0;
        }
        const_buffer_stream& operator=(const_buffer_stream&& rhs) {
            m_begin=rhs.m_begin;
            m_current=rhs.m_current;
            m_size=rhs.m_size;
            rhs.m_begin=rhs.m_current=nullptr;
            rhs.m_size=0;
            return *this;
        }
        void set(const uint8_t* buffer,size_t size) {
            m_begin = m_current = buffer;
            m_size = size;
            if(nullptr==buffer) m_size=0;
        }
        virtual int getc() {
            if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
                return -1;
#if defined(ARDUINO) && !defined(ESP32)
            return pgm_read_byte(m_current++);
#else
            return *(m_current++);
#endif
        }
        virtual int putc(int value) {
            return -1;
        }
        virtual stream_caps caps() const {
            stream_caps s;
            s.read = s.seek= (nullptr!=m_current);
            s.write = 0;
        
            return s;
        }
        virtual size_t read(uint8_t* destination,size_t size) {
            if(nullptr==m_current || nullptr==destination || size==0)
                return 0;
            size_t offs=m_current-m_begin;
            if(offs>=m_size)
                return 0;
            if(size+offs>m_size) 
                size = (m_size-offs);
#if defined(ARDUINO) && !defined(ESP32)
            offs=size;
            while(offs--) {
                *destination++=pgm_read_byte(m_current++);
            }
#else
            memcpy(destination,m_current,size);
            m_current+=size;
#endif
            return size;
        }
        
        virtual size_t write(const uint8_t* source,size_t size) {
            return 0;
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
#ifdef ARDUINO

    class arduino_stream final : public stream {
        Stream* m_stream;
        arduino_stream(const arduino_stream& rhs) = delete;
        arduino_stream& operator=(const arduino_stream& rhs)=delete;
    public:
        arduino_stream(Stream* stream) : m_stream(stream) {
        }
        arduino_stream(arduino_stream&& rhs) : m_stream(rhs.m_stream) {
            rhs.m_stream=nullptr;
        }
        arduino_stream& operator=(arduino_stream&& rhs) {
            m_stream=rhs.m_stream;
            rhs.m_stream=nullptr;
            return *this;
        }
        virtual size_t read(uint8_t* destination,size_t size) {
            if(nullptr==m_stream) return 0;
            return m_stream->readBytes(destination,size);
        }
        virtual int getc() {
            if(nullptr==m_stream) return -1;
            return m_stream->read();
        }
        virtual size_t write(const uint8_t* source,size_t size) {
            if(nullptr==m_stream) return 0;
            return m_stream->write(source,size);
        }
        virtual int putc(int value) {
            if(nullptr==m_stream) return 0;
            return m_stream->write((uint8_t)value);
        } 
        virtual unsigned long long seek(long long position, seek_origin origin=seek_origin::start) {
            return 0;
        }
        virtual stream_caps caps() const {
            stream_caps c;
            c.read = 1;
            c.write = 1;
            c.seek = 0;
            return c;

            
        }
    };
#endif

    class file_stream final : public stream {
#ifdef ARDUINO
        File& m_file;
#else
        FILE* m_fd;
#endif
        stream_caps m_caps;
        file_stream(const file_stream& rhs) = delete;
        file_stream& operator=(const file_stream& rhs)=delete;
    public:
#ifdef ARDUINO
        file_stream(File& file) : m_file(file){
            if(!m_file) {
                m_caps.read=0;
                m_caps.write=0;
                m_caps.seek=0;
            } else {
                m_caps.read=1;
                m_caps.write=1;
                m_caps.seek=1;
            }
        }
        file_stream(file_stream&& rhs) : m_file(rhs.m_file),m_caps(rhs.m_caps) {
            rhs.m_caps={0,0,0};
        }
        file_stream& operator=(file_stream&& rhs) {
            m_file=rhs.m_file;
            m_caps=rhs.m_caps;
            rhs.m_caps={0,0,0};
            return *this;
        }
        ~file_stream() {
        }
        File& handle() const {
            return m_file;
        }
        virtual size_t read(uint8_t* destination,size_t size) {
            if(!m_file) return 0;
            //return m_file.read(destination,size);
            if(0!=m_file.readBytes((char*)destination,size)) {
                return size;
            }
            return 0;
        }
        virtual int getc() {
            if(!m_file) return -1;
            return m_file.read();
        }
        virtual size_t write(const uint8_t* source,size_t size) {
            if(!m_file) return 0;
            size_t s= m_file.write(source,size);
            //m_file.flush();
            return s;
        }
        virtual int putc(int value) {
            if(!m_file) return 0;
            size_t s = m_file.write((uint8_t)value);
            //m_file.flush();
            return s;
        } 
        virtual unsigned long long seek(long long position, seek_origin origin=seek_origin::start) {
            
            switch(origin) {
                case seek_origin::start:
                    m_file.seek((uint32_t)position);
                    break;
                case seek_origin::current:
                    if(0!=position) {
                        m_file.seek(uint32_t(position+m_file.position()));
                    }
                    break;
                case seek_origin::end:
                    m_file.seek(uint32_t(m_file.size()+position));
                    break;
            }
            return m_file.position();
        }
#else
        file_stream(const char* name,file_mode mode=file_mode::read) : m_caps({0,0,0}) {
            if((int)file_mode::append==((int)mode & (int)file_mode::append)) {
                if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                    m_fd = fopen(name,"ab+");
                    if(nullptr!=m_fd) {
                        m_caps.read=1;
                        m_caps.write=1;
                        m_caps.seek=1;
                    }
                    return;
                }
                m_fd = fopen(name,"ab");
                if(nullptr!=m_fd) {
                    m_caps.read=0;
                    m_caps.write=1;
                    m_caps.seek=1;
                }
                return;
            }
            if((int)file_mode::write==((int)mode & (int)file_mode::write)) {
                if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                    m_fd = fopen(name,"wb+");
                    if(nullptr!=m_fd) {
                        m_caps.read=1;
                        m_caps.write=1;
                        m_caps.seek=1;
                    }
                    return;
                }
                m_fd = fopen(name,"wb");
                if(nullptr!=m_fd) {
                    m_caps.read=0;
                    m_caps.write=1;
                    m_caps.seek=1;
                }
                return;
            }
            if((int)file_mode::read==((int)mode & (int)file_mode::read)) {
                m_fd = fopen(name,"rb");
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
            close();
        }
        FILE* handle() const {
            return m_fd;
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
        void close() {
            if(nullptr!=m_fd)
                fclose(m_fd);
        }
#endif
        virtual stream_caps caps() const {
            return m_caps;
        }
    };
    class stream_reader_base {
    protected:
        stream* m_stream;
        stream_reader_base(io::stream* stream) : m_stream(stream) {

        }
    public:
        inline bool initialized() const {
            return m_stream!=nullptr;
        }
        inline stream* base_stream() const {
            return m_stream;
        }
    };
    class stream_reader_le : public stream_reader_base {
     public:
        stream_reader_le(io::stream* stream) : stream_reader_base(stream) {

        }
        bool read(uint8_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
        bool read(uint16_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
        bool read(uint32_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
#if HTCW_MAX_WORD >= 64
        bool read(uint64_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
#endif
        bool read(int8_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
        bool read(int16_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
        bool read(int32_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
#if HTCW_MAX_WORD >= 64
        bool read(int64_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_le(*out_value);
            return true;
        }
#endif

    };
    class stream_reader_be : public stream_reader_base {
    public:
        stream_reader_be(io::stream* stream) : stream_reader_base(stream) {

        }
        bool read(uint8_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
        bool read(uint16_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
        bool read(uint32_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
#if HTCW_MAX_WORD >= 64
        bool read(uint64_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
#endif
        bool read(int8_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
        bool read(int16_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
        bool read(int32_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
#if HTCW_MAX_WORD >= 64
        bool read(int64_t* out_value) const {
            if(nullptr==m_stream || nullptr==out_value) {
                return false;
            }
            if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
                return false;
            }
            *out_value = bits::from_be(*out_value);
            return true;
        }
#endif
    };
}
#endif