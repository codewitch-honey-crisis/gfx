#include <bits.hpp>
#include <io_stream.hpp>
namespace io {
    buffer_stream::buffer_stream() : m_begin(nullptr),m_current(nullptr),m_size(0) {}
    buffer_stream::buffer_stream(uint8_t* buffer, size_t size) :m_begin(buffer),m_current(buffer),m_size(size) {
        if(nullptr==buffer) m_size=0;
    }
    buffer_stream::buffer_stream(buffer_stream&& rhs) : m_begin(rhs.m_begin), m_current(rhs.m_current), m_size(rhs.m_size) {
        rhs.m_begin=rhs.m_current=nullptr;
        rhs.m_size=0;
    }
    buffer_stream& buffer_stream::operator=(buffer_stream&& rhs) {
        m_begin=rhs.m_begin;
        m_current=rhs.m_current;
        m_size=rhs.m_size;
        rhs.m_begin=rhs.m_current=nullptr;
        rhs.m_size=0;
        return *this;
    }
    uint8_t* buffer_stream::handle() {
        return m_begin;
    }
    void buffer_stream::set(uint8_t* buffer,size_t size) {
        m_begin = m_current = buffer;
        m_size = size;
        if(nullptr==buffer) m_size=0;
    }
    int buffer_stream::getc() {
        if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
            return -1;
        return *(m_current++);
    }
    int buffer_stream::putc(int value) {
        if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
            return -1;
        return *(m_current++)=(uint8_t)value;
    }
    stream_caps buffer_stream::caps() const {
            stream_caps s;
            s.read = s.write = s.seek = (nullptr!=m_current);
            return s;
        }
    size_t buffer_stream::read(uint8_t* destination,size_t size) {
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
    
    size_t buffer_stream::write(const uint8_t* source,size_t size) {
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
    
    unsigned long long buffer_stream::seek(long long position,seek_origin origin)  {
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
                return seek(m_size+position,seek_origin::start);
            default:
                return offs;
        }
        m_current=m_begin+p;
        return m_current-m_begin;   
    }

    const_buffer_stream::const_buffer_stream() : m_begin(nullptr),m_current(nullptr),m_size(0) {}
    const_buffer_stream::const_buffer_stream(const uint8_t* buffer,size_t size) :m_begin(buffer),m_current(buffer),m_size(size) {
        if(nullptr==buffer) m_size=0;
    }
    const_buffer_stream::const_buffer_stream(const_buffer_stream&& rhs) : m_begin(rhs.m_begin), m_current(rhs.m_current), m_size(rhs.m_size) {
        rhs.m_begin=rhs.m_current=nullptr;
        rhs.m_size=0;
    }
    const_buffer_stream& const_buffer_stream::operator=(const_buffer_stream&& rhs) {
        m_begin=rhs.m_begin;
        m_current=rhs.m_current;
        m_size=rhs.m_size;
        rhs.m_begin=rhs.m_current=nullptr;
        rhs.m_size=0;
        return *this;
    }
    const uint8_t* const_buffer_stream::handle() const {
        return m_begin;
    }
    void const_buffer_stream::set(const uint8_t* buffer,size_t size) {
        m_begin = m_current = buffer;
        m_size = size;
        if(nullptr==buffer) m_size=0;
    }
    int const_buffer_stream::getc() {
        if(nullptr==m_current || (size_t)(m_current-m_begin)>=m_size)
            return -1;
#if defined(ARDUINO) && !defined(ESP32)
        return pgm_read_byte(m_current++);
#else
        return *(m_current++);
#endif
    }
    int const_buffer_stream::putc(int value) {
        return -1;
    }
    stream_caps const_buffer_stream::caps() const {
        stream_caps s;
        s.read = s.seek= (nullptr!=m_current);
        s.write = 0;
    
        return s;
    }
    size_t const_buffer_stream::read(uint8_t* destination,size_t size) {
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
    
    size_t const_buffer_stream::write(const uint8_t* source,size_t size) {
        return 0;
    }
    
    unsigned long long const_buffer_stream::seek(long long position,seek_origin origin) {
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
    
#ifdef ARDUINO
    arduino_stream::arduino_stream(Stream* stream) : m_stream(stream) {
    }
    arduino_stream::arduino_stream(arduino_stream&& rhs) : m_stream(rhs.m_stream) {
        rhs.m_stream=nullptr;
    }
    arduino_stream& arduino_stream::operator=(arduino_stream&& rhs) {
        m_stream=rhs.m_stream;
        rhs.m_stream=nullptr;
        return *this;
    }
    size_t arduino_stream::read(uint8_t* destination,size_t size) {
        if(nullptr==m_stream) return 0;
        return m_stream->readBytes(destination,size);
    }
    int arduino_stream::getc() {
        if(nullptr==m_stream) return -1;
        return m_stream->read();
    }
    size_t arduino_stream::write(const uint8_t* source,size_t size) {
        if(nullptr==m_stream) return 0;
        return m_stream->write(source,size);
    }
    int arduino_stream::putc(int value) {
        if(nullptr==m_stream) return 0;
        return m_stream->write((uint8_t)value);
    } 
    unsigned long long arduino_stream::seek(long long position, seek_origin origin) {
        return 0;
    }
    stream_caps arduino_stream::caps() const {
        stream_caps c;
        c.read = 1;
        c.write = 1;
        c.seek = 0;
        return c;
    }

#endif

#ifndef IO_NO_FS
#ifdef ARDUINO
    file_stream::file_stream(File& file) : m_file(file){
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
    file_stream::file_stream(file_stream&& rhs) : m_file(rhs.m_file),m_caps(rhs.m_caps) {
        rhs.m_caps={0,0,0};
    }
    file_stream& file_stream::operator=(file_stream&& rhs) {
        m_file=rhs.m_file;
        m_caps=rhs.m_caps;
        rhs.m_caps={0,0,0};
        return *this;
    }
    file_stream::~file_stream() {
    }
    File& file_stream::handle() const {
        return m_file;
    }
    void file_stream::set(File& file) {
        if(!file) {
            m_caps.read = m_caps.write = m_caps.seek = 0;
        } else {
            m_caps.read = m_caps.write = m_caps.seek = 1;
        }
        m_file = file;
    }
    size_t file_stream::read(uint8_t* destination,size_t size) {
        if(!m_file) return 0;
        if(0!=m_file.readBytes((char*)destination,size)) {
            return size;
        }
        return 0;
    }
    int file_stream::getc() {
        if(!m_file) return -1;
        return m_file.read();
    }
    size_t file_stream::write(const uint8_t* source,size_t size) {
        if(!m_file) return 0;
        size_t s= m_file.write(source,size);
        //m_file.flush();
        return s;
    }
    int file_stream::putc(int value) {
        if(!m_file) return 0;
        size_t s = m_file.write((uint8_t)value);
        //m_file.flush();
        return s;
    } 
    unsigned long long file_stream::seek(long long position, seek_origin origin) {    
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
    void file_stream::close() {
        if(!m_file) {
            return;;
        }
        m_file.close();
    }
#else
    file_stream::file_stream(const char* name,file_mode mode) : m_fd(nullptr), m_caps({0,0,0}) {
        set(name,mode);
    }
    file_stream::file_stream(file_stream&& rhs) : m_fd(rhs.m_fd),m_caps(rhs.m_caps) {
        rhs.m_fd=nullptr;
        rhs.m_caps={0,0,0};
    }
    file_stream& file_stream::operator=(file_stream&& rhs) {
        m_fd=rhs.m_fd;
        m_caps=rhs.m_caps;
        rhs.m_fd=nullptr;
        rhs.m_caps={0,0,0};
        return *this;
    }
    file_stream::~file_stream() {
        close();
    }
    FILE* file_stream::handle() const {
        return m_fd;
    }
    void file_stream::set(const char* name,file_mode mode) {
        close();
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
    size_t file_stream::read(uint8_t* destination,size_t size) {
        if(0==m_caps.read) return 0;
        return fread(destination,1,size,m_fd);
    }
    int file_stream::getc() {
        if(0==m_caps.read) return -1;
        return fgetc(m_fd);
    }
    size_t file_stream::write(const uint8_t* source,size_t size) {
        if(0==m_caps.write) return 0;
        return fwrite(source,1,size,m_fd);
    }
    int file_stream::putc(int value) {
        if(0==m_caps.write) return -1;
        return fputc(value,m_fd);
    } 
    unsigned long long file_stream::seek(long long position, seek_origin origin) {
        if(0==m_caps.seek) return 0; // should always be true unless uninitialized
        // on error we still need to report the current position
        // so we continue regardless
        if(position!=0||origin!=seek_origin::current)
            fseek(m_fd,(long int)position,(int)origin);
        return ftell(m_fd);
    }
    void file_stream::close() {
        if(nullptr!=m_fd) {
            m_caps = {0,0,0};
            fclose(m_fd);
            m_fd=nullptr;
        }
    }
#endif
    stream_caps file_stream::caps() const {
        return m_caps;
    }   
#endif

    stream_reader_le::stream_reader_le(io::stream* stream) : stream_reader_base(stream) {}
    bool stream_reader_le::read(uint8_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_le(*out_value);
        return true;
    }
    bool stream_reader_le::read(uint16_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_le(*out_value);
        return true;
    }
    bool stream_reader_le::read(uint32_t* out_value) const {
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
    bool stream_reader_le::read(uint64_t* out_value) const {
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
    bool stream_reader_le::read(int8_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_le(*out_value);
        return true;
    }
    bool stream_reader_le::read(int16_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_le(*out_value);
        return true;
    }
    bool stream_reader_le::read(int32_t* out_value) const {
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
    bool stream_reader_le::read(int64_t* out_value) const {
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

    stream_reader_be::stream_reader_be(io::stream* stream) : stream_reader_base(stream) {

    }
    bool stream_reader_be::read(uint8_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_be(*out_value);
        return true;
    }
    bool stream_reader_be::read(uint16_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_be(*out_value);
        return true;
    }
    bool stream_reader_be::read(uint32_t* out_value) const {
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
    bool stream_reader_be::read(uint64_t* out_value) const {
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
    bool stream_reader_be::read(int8_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_be(*out_value);
        return true;
    }
    bool stream_reader_be::read(int16_t* out_value) const {
        if(nullptr==m_stream || nullptr==out_value) {
            return false;
        }
        if(sizeof(*out_value)!=m_stream->read((uint8_t*)out_value,sizeof(*out_value))) {
            return false;
        }
        *out_value = bits::from_be(*out_value);
        return true;
    }
    bool stream_reader_be::read(int32_t* out_value) const {
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
    bool stream_reader_be::read(int64_t* out_value) const {
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
}