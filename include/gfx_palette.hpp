#ifndef HTCW_GFX_PALETTE_HPP
#define HTCW_GFX_PALETTE_HPP
#include "gfx_core.hpp"
#include "gfx_pixel.hpp"
#include <stdlib.h>
namespace gfx {
        // represents a palette/CLUT for indexed pixels
    template<typename PixelType,typename MappedPixelType>
    struct palette final {
        static_assert(PixelType::template has_channel_names<channel_name::index>::value,"Pixel must be indexed");
        static_assert(!MappedPixelType::template has_channel_names<channel_name::index>::value,"Mapped pixel must not be indexed");
        using type = palette;
        using pixel_type = PixelType;
        using mapped_pixel_type = MappedPixelType;
    private:
        using tindex = typename pixel_type::template channel_index_by_name<channel_name::index>;
        using tch = typename pixel_type::template channel_by_index_unchecked<tindex::value>;
    public:
        constexpr static const bool writable = true;
        constexpr static const size_t size = tch::max-tch::min+1;
    private:
        mapped_pixel_type m_colors[size];
    public:
       palette() {}
        palette(const palette& rhs)  {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
        }
        palette& operator=(const palette& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
            return *this;
        }
        palette(palette&& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
        }
        palette& operator=(palette&& rhs) {
            memcpy(m_colors,rhs.m_colors,size*sizeof(mapped_pixel_type));
            return *this;
        }
        ~palette() {}
        // maps an indexed pixel to a color value
        gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            const size_t i = pixel.template channel_unchecked<tindex::value>()-tch::min;
            *mapped_pixel=m_colors[i];
            return gfx_result::success;
        }
        // sets an indexed pixel to a mapped color value
        gfx_result map(pixel_type pixel,mapped_pixel_type mapped_pixel) {
            const size_t i = pixel.template channel_unchecked<tindex::value>()-tch::min;
            m_colors[i]=mapped_pixel;
            return gfx_result::success;
        }
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            if(0==size) {
                return gfx_result::invalid_argument;
            }
            double least = m_colors[0].difference(mapped_pixel);
            if(0.0==least) {
                pixel->native_value = 0;
                return gfx_result::success;
            }
            int ii=0;
            for(int i = 1;i<size;++i) {
                double cmp = m_colors[i].difference(mapped_pixel);
                if(0.0==cmp) {
                    ii=i;
                    least = 0.0;
                    break;
                }
                if(cmp<least) {
                    least = cmp;
                    ii=i;
                }
            }
            pixel->native_value=ii;
            //printf("nearest (native palette) was %d\r\n",ii);
            return gfx_result::success;
        }
    };
    // specialization for unindexed pixels
    template<typename PixelType>
    struct palette<PixelType,PixelType> final {
        static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"Pixel must not be indexed");
        using type = palette;
        using pixel_type = PixelType;
        using mapped_pixel_type = PixelType;
        palette() {}
        palette(const palette& rhs)=default;
        palette& operator=(const palette& rhs)=default;
        palette(palette&& rhs)=default;
        palette& operator=(palette&& rhs)=default;
        ~palette() {}
        constexpr static const bool writable = false;
        constexpr static const size_t size = 0;
        // maps an indexed pixel to a color value
        inline gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            *mapped_pixel=pixel;
            return gfx_result::success;
        }
        /*
        // sets an indexed pixel to a mapped color value
        inline gfx_result map(pixel_type pixel,mapped_pixel_type mapped_pixel) {
            return gfx_result::not_supported;
        }
        */
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
            //printf("nearest\r\n");
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            pixel->native_value = mapped_pixel.native_value;
            return gfx_result::success;
        }
    };
    template<typename Destination,typename Source>
    inline static gfx_result convert_palette(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result, const typename Destination::pixel_type* background=nullptr);
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_to(const Target& target,typename Target::pixel_type pixel, PixelType* result, const PixelType* background=nullptr);
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_from(Target& target,PixelType pixel, typename Target::pixel_type* result, const typename Target::pixel_type* background=nullptr) ;
    namespace helpers {
        template<typename Target,typename PixelType, bool Indexed>
        struct palette_mapper_impl {
        };
        template<typename Target,typename PixelType>
        struct palette_mapper_impl<Target,PixelType,false> {
            inline static gfx_result pixel_to_indexed(const Target& target,PixelType pixel,const typename Target::pixel_type* background,  typename Target::pixel_type* indexed) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                typename Target::pixel_type px;
                gfx_result r = convert(pixel, &px,background);
                if(gfx_result::success!=r) {
                    return r;
                }
                indexed->native_value = px.native_value;
                return gfx_result::success;
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel,const PixelType* background, PixelType* mapped) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                PixelType px;
                gfx_result r = convert(pixel,&px,background);
                if(gfx_result::success!=r) {
                    return r;
                }
                mapped->native_value = px.native_value;
                return gfx_result::success;
            }
        };
        template<typename Target>
        struct palette_mapper_impl<Target,typename Target::pixel_type,true> {
            inline static gfx_result pixel_to_indexed(const Target& target,typename Target::pixel_type pixel,const typename Target::pixel_type* background, typename Target::pixel_type* indexed) { 
                if(nullptr==background) {
                    indexed->native_value = pixel.native_value;
                    return gfx_result::success;
                }
                typename Target::palette_type::mapped_pixel_type mpx;
                
                gfx_result r;
                //r= palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,pixel,nullptr,&mpx);
                r=convert_palette_to(target,pixel,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                typename Target::palette_type::mapped_pixel_type mbg;
                //r= palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,*background,nullptr,&mbg);
                r=convert_palette_to(target,*background,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert(mpx,&mpx,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                return palette_mapper_impl<Target,typename Target::palette_type::mapped_pixel_type,true>::pixel_to_indexed(target,mpx,nullptr,indexed);
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel,const typename Target::pixel_type* background,typename Target::pixel_type* mapped) {
                return pixel_to_indexed(target,pixel,background,mapped);
            }
        };
        template<typename Target,typename PixelType>
        struct palette_mapper_impl<Target,PixelType,true> {
            inline static gfx_result pixel_to_indexed(const Target& target,PixelType pixel,const typename Target::pixel_type* background,typename Target::pixel_type* indexed) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                const typename Target::palette_type* pal=target.palette();
                if(nullptr==pal) {
                    return gfx_result::no_palette;
                }
                gfx_result r;
                typename Target::palette_type::mapped_pixel_type mpx;
                if(nullptr==background)  {
                    r = convert(pixel,&mpx);    
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    return pal->nearest(mpx,indexed);
                }
                typename Target::palette_type::mapped_pixel_type mbg;
                r=pal->map(*background,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                r=convert(pixel,&mpx,&mbg);
                if(gfx_result::success!=r) {
                    return r;
                }
                return pal->nearest(mpx,indexed);
            }
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel, const PixelType* background, PixelType* mapped) {
                static_assert(!PixelType::template has_channel_names<channel_name::index>::value,"PixelType must not be indexed");
                const typename Target::palette_type* pal=target.palette();
                if(nullptr==pal) {
                    return gfx_result::no_palette;
                }
                typename Target::palette_type::mapped_pixel_type mpx;
                gfx_result r= pal->map(pixel,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                return convert(mpx,mapped);
            }
        };
        template<typename Target,typename PixelType>
        struct palette_mapper {
            inline static gfx_result indexed_to_pixel(const Target& target,typename Target::pixel_type pixel, PixelType* mapped,const PixelType* background=nullptr) {
                using thas_index = typename Target::pixel_type::template has_channel_names<channel_name::index>;
                return helpers::palette_mapper_impl<Target,PixelType,thas_index::value>::indexed_to_pixel(target,pixel,background,mapped);
            }
            
            inline static gfx_result pixel_to_indexed(Target& target,PixelType pixel, typename Target::pixel_type* indexed,const typename Target::pixel_type* background=nullptr) {
                using thas_index = typename Target::pixel_type::template has_channel_names<channel_name::index>;
                return helpers::palette_mapper_impl<Target,PixelType,thas_index::value>::pixel_to_indexed(target,pixel,background,indexed);
            }
        };
        template<typename Destination,typename Source,bool IsDestinationIndexed,bool IsSourceIndexed>
        struct palette_converter {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                return gfx::convert(pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,true,false> {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                static_assert(!Source::pixel_type::template has_channel_names<channel_name::index>::value,"Source pixel type must not be indexed");
                return palette_mapper<Destination,typename Source::pixel_type>::pixel_to_indexed(destination,pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,false,true> {
            inline static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                static_assert(!Destination::pixel_type::template has_channel_names<channel_name::index>::value,"Destination pixel type must not be indexed");
                return palette_mapper<Source,typename Destination::pixel_type>::indexed_to_pixel(source,pixel,result,background);
            }
        };
        template<typename Destination,typename Source>
        struct palette_converter<Destination,Source,true,true> {
            static gfx_result convert(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result,const typename Destination::pixel_type* background = nullptr) {
                gfx_result r;
                rgb_pixel<HTCW_MAX_WORD> rgb;
                if(nullptr==background) {
                    //r=convert_palette_to(source,pixel,&rgb,nullptr);
                    r= palette_mapper<Source,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(source,pixel,&rgb,nullptr);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    return palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::pixel_to_indexed(destination,rgb,result,nullptr);
                }
                rgb_pixel<HTCW_MAX_WORD> bg;
                r= palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(destination,*background,&rgb,&bg);
                r= palette_mapper<Source,rgb_pixel<HTCW_MAX_WORD>>::indexed_to_pixel(source,pixel,&rgb,nullptr);
                if(gfx_result::success!=r) {
                    return r;
                }
                return palette_mapper<Destination,rgb_pixel<HTCW_MAX_WORD>>::pixel_to_indexed(destination,rgb,result,nullptr);
            }
        }; 
    }
    template<typename Destination,typename Source>
    inline static gfx_result convert_palette(Destination& destination, const Source& source, typename Source::pixel_type pixel, typename Destination::pixel_type* result, const typename Destination::pixel_type* background) {
        using tshas_index = typename Source::pixel_type::template has_channel_names<channel_name::index>;
        using tdhas_index = typename Destination::pixel_type::template has_channel_names<channel_name::index>;
        return helpers::palette_converter<Destination,Source,tdhas_index::value, tshas_index::value>::convert( destination,source,pixel,result,background);
    }
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_to(const Target& target,typename Target::pixel_type pixel, PixelType* result, const PixelType* background) {
        return helpers::palette_mapper<Target,PixelType>::indexed_to_pixel(target,pixel,result,background);
    }
    template<typename Target,typename PixelType>
    inline static gfx_result convert_palette_from(Target& target,PixelType pixel, typename Target::pixel_type* result, const typename Target::pixel_type* background) {
        //using tshas_index = typename PixelType::template has_channel_names<channel_name::index>;
        //static_assert(!tshas_index::value,"PixelType must not be indexed");
        return helpers::palette_mapper<Target,PixelType>::pixel_to_indexed(target,pixel,result,background);
    }

    template<typename MappedPixelType> 
    struct ega_palette {
        static_assert(!MappedPixelType::template has_channel_names<channel_name::index>::value,"Mapped pixel must not be indexed");
    private:
        constexpr static gfx_result index_to_mapped(int idx,MappedPixelType* result) {
            const uint8_t red   = 85 * (((idx >> 1) & 2) | ((idx >> 5) & 1));
            const uint8_t green = 85 * (( idx       & 2) | ((idx >> 4) & 1));
            const uint8_t blue  = 85 * (((idx << 1) & 2) | ((idx >> 3) & 1));
            return convert(rgb_pixel<24>(red,green,blue),result);
        }
    public:
        using type = ega_palette;
        using pixel_type = indexed_pixel<4>;
        using mapped_pixel_type = MappedPixelType;
        constexpr static const bool writable = false;
        constexpr static const size_t size = 16;
        gfx_result map(pixel_type pixel,mapped_pixel_type* mapped_pixel) const {
            return index_to_mapped(pixel.channel<channel_name::index>(),mapped_pixel);
        }
        gfx_result nearest(mapped_pixel_type mapped_pixel,pixel_type* pixel) const {
            
            if(nullptr==pixel) {
                return gfx_result::invalid_argument;
            }
            mapped_pixel_type mpx;
            gfx_result r = index_to_mapped(0,&mpx);
            if(gfx_result::success!=r) {
                return r;
            }
            double least = mpx.difference(mapped_pixel);
            if(0.0==least) {
                pixel->native_value = 0;
                return gfx_result::success;
            }
            int ii=0;
            for(int i = 1;i<size;++i) {
                r=index_to_mapped(i,&mpx);
                if(gfx_result::success!=r) {
                    return r;
                }
                double cmp = mpx.difference(mapped_pixel);
                if(0.0==cmp) {
                    ii=i;
                    least = 0.0;
                    break;
                }
                if(cmp<least) {
                    least = cmp;
                    ii=i;
                }
            }
            pixel->channel<channel_name::index>(ii);
            //printf("nearest was %d\r\n",ii);
            return gfx_result::success;
        }
    };
    
    namespace helpers {
        
        template<typename ValueType, unsigned K = 3>
        class kd_tree
        {
        public:
            
            struct kd_point
            {
                double coord[K];

                kd_point() { }

                kd_point(double a,double b,double c)
                {
                    coord[0] = a; coord[1] = b; coord[2] = c;
                }

                kd_point(double v[K])
                {
                    for(unsigned n=0; n<K; ++n)
                        coord[n] = v[n];
                }

                bool operator==(const kd_point& b) const
                {
                    for(unsigned n=0; n<K; ++n)
                        if(coord[n] != b.coord[n]) return false;
                    return true;
                }
                double sqrdist(const kd_point& b) const
                {
                    double result = 0;
                    for(unsigned n=0; n<K; ++n)
                        { double diff = coord[n] - b.coord[n];
                        result += diff*diff; }
                    return result;
                }
            };
        private:
            struct kd_rect
            {
                kd_point min, max;

                kd_point bound(const kd_point& t) const
                {
                    kd_point p;
                    for(unsigned i=0; i<K; ++i)
                        if(t.coord[i] <= min.coord[i])
                            p.coord[i] = min.coord[i];
                        else if(t.coord[i] >= max.coord[i])
                            p.coord[i] = max.coord[i];
                        else
                            p.coord[i] = t.coord[i];
                    return p;
                }
                void make_infinite()
                {
                    for(unsigned i=0; i<K; ++i)
                    {
                        min.coord[i] = -INFINITY;
                        max.coord[i] =  INFINITY;
                    }
                }
            };
            struct kd_pair {
                ValueType first;
                double second;
            };
            struct kd_node
            {
                kd_point k;
                ValueType       v;
                kd_node  *left, *right;
            public:
                kd_node() : k(),v(),left(0),right(0) { }
                kd_node(const kd_point& kk, const ValueType& vv) : k(kk), v(vv), left(0), right(0) { }

                virtual ~kd_node() { delete (left); delete (right); }

                static kd_node* ins( const kd_point& key, const ValueType& val,
                                    kd_node*& t, int lev)
                {
                    if(!t)
                        return (t = new kd_node(key, val));
                    else if(key == t->k)
                        return 0; /* key duplicate */
                    else if(key.coord[lev] > t->k.coord[lev])
                        return ins(key, val, t->right, (lev+1)%K);
                    else
                        return ins(key, val, t->left,  (lev+1)%K);
                }
                struct nearest
                {
                    const kd_node* kd;
                    double        dist_sqd;
                };
                // Method nearest Neighbor from Andrew Moore's thesis. Numbered
                // comments are direct quotes from there. Step "SDL" is added to
                // make the algorithm work correctly.
                static void nnbr(const kd_node* kd, const kd_point& target,
                                kd_rect& hr, // in-param and temporary; not an out-param.
                                int lev,
                                nearest& nearest)
                {
                    // 1. if kd is empty then set dist-sqd to infinity and exit.
                    if (!kd) return;

                    // 2. s := split field of kd
                    int s = lev % K;

                    // 3. pivot := dom-elt field of kd
                    const kd_point& pivot = kd->k;
                    double pivot_to_target = pivot.sqrdist(target);

                    // 4. Cut hr into to sub-hyperrectangles left-hr and right-hr.
                    //    The cut plane is through pivot and perpendicular to the s
                    //    dimension.
                    kd_rect& left_hr = hr; // optimize by not cloning
                    kd_rect right_hr = hr;
                    left_hr.max.coord[s]  = pivot.coord[s];
                    right_hr.min.coord[s] = pivot.coord[s];

                    // 5. target-in-left := target_s <= pivot_s
                    bool target_in_left = target.coord[s] < pivot.coord[s];

                    const kd_node* nearer_kd;
                    const kd_node* further_kd;
                    kd_rect nearer_hr;
                    kd_rect further_hr;

                    // 6. if target-in-left then nearer is left, further is right
                    if (target_in_left) {
                        nearer_kd = kd->left;
                        nearer_hr = left_hr;
                        further_kd = kd->right;
                        further_hr = right_hr;
                    }
                    // 7. if not target-in-left then nearer is right, further is left
                    else {
                        nearer_kd = kd->right;
                        nearer_hr = right_hr;
                        further_kd = kd->left;
                        further_hr = left_hr;
                    }

                    // 8. Recursively call nearest Neighbor with parameters
                    //    (nearer-kd, target, nearer-hr, max-dist-sqd), storing the
                    //    results in nearest and dist-sqd
                    nnbr(nearer_kd, target, nearer_hr, lev + 1, nearest);

                    // 10. A nearer point could only lie in further-kd if there were some
                    //     part of further-hr within distance sqrt(max-dist-sqd) of
                    //     target.  If this is the case then
                    const kd_point closest = further_hr.bound(target);
                    if (closest.sqrdist(target) < nearest.dist_sqd)
                    {
                        // 10.1 if (pivot-target)^2 < dist-sqd then
                        if (pivot_to_target < nearest.dist_sqd)
                        {
                            // 10.1.1 nearest := (pivot, range-elt field of kd)
                            nearest.kd = kd;
                            // 10.1.2 dist-sqd = (pivot-target)^2
                            nearest.dist_sqd = pivot_to_target;
                        }

                        // 10.2 Recursively call nearest Neighbor with parameters
                        //      (further-kd, target, further-hr, max-dist_sqd)
                        nnbr(further_kd, target, further_hr, lev + 1, nearest);
                    }
                    // SDL: otherwise, current point is nearest
                    else if (pivot_to_target < nearest.dist_sqd)
                    {
                        nearest.kd       = kd;
                        nearest.dist_sqd = pivot_to_target;
                    }
                }
            private:
                void operator=(const kd_node&);
            public:
                kd_node(const kd_node& b)
                    : k(b.k), v(b.v),
                    left( b.left ? new kd_node(*b.left) : 0),
                    right( b.right ? new kd_node(*b.right) : 0 ) { }
            };
        private:
            kd_node* m_root;
        public:
            kd_tree() : m_root(0) { }
            virtual ~kd_tree() { delete (m_root); }

            bool insert(const kd_point& key, const ValueType& val)
            {
                return kd_node::ins(key, val, m_root, 0);
            }

            const kd_pair nearest(const kd_point& key) const
            {
                kd_rect hr;
                hr.make_infinite();

                typename kd_node::nearest nn;
                nn.kd       = 0;
                nn.dist_sqd = INFINITY;
                kd_node::nnbr(m_root, key, hr, 0, nn);
                if(!nn.kd) return { ValueType(), INFINITY };
                return { nn.kd->v, nn.dist_sqd };
            }
        public:
            kd_tree& operator=(const kd_tree&b)
            {
                if(this != &b)
                {
                    if(m_root) delete (m_root);
                    m_root = b.m_root ? new kd_node(*b.m_root) : 0;
                }
                return *this;
            }
            kd_tree(const kd_tree& b)
                : m_root( b.m_root ? new kd_node(*b.m_root) : 0 ){ }
        };
        // You'll get warnings that these are unused, but they are used by driver code:
        struct dither_color {
        /* 8x8 threshold map (note: the patented pattern dithering algorithm uses 4x4) */
            static const unsigned char* threshold_map;
            static const double* threshold_map_fast;
            static double color_compare(int r1,int g1,int b1, int r2,int g2,int b2);
            static unsigned* dither_luminosity_cache;
            static gfx_result unprepare();
            template<typename PaletteType>
            static gfx_result prepare(const PaletteType* palette) {
                if(nullptr==palette) {
                    return gfx_result::invalid_argument;
                }
                gfx_result r= unprepare();
                if(gfx_result::success!=r) {
                    return r;
                }
                const size_t size = PaletteType::size;
                dither_luminosity_cache = new unsigned[size];
                if(nullptr==dither_luminosity_cache) {
                    return gfx_result::out_of_memory;
                }
                unsigned*p=(unsigned*)dither_luminosity_cache;
                for(int i = 0;i<size;++i) {
                    typename PaletteType::pixel_type ipx(i);
                    typename PaletteType::mapped_pixel_type mpx;
                    rgb_pixel<24> rgb888;
                    r=palette->map(ipx,&mpx);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    r=convert(mpx,&rgb888);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    int r = rgb888.template channel<channel_name::R>();
                    int g = rgb888.template channel<channel_name::G>();
                    int b = rgb888.template channel<channel_name::B>();
                    *p++ =  r*299 + g*587 + b*114;
                }
                return gfx_result::success;
            }
            static double mixing_error(int r,int g,int b,
                            int r0,int g0,int b0,
                            int r1,int g1,int b1,
                            int r2,int g2,int b2,
                            double ratio);
            struct mixing_plan_data_fast
            {
                unsigned colors[2];
                double ratio; /* 0 = always index1, 1 = always index2, 0.5 = 50% of both */
            };
            template<typename PaletteType>
            static gfx_result mixing_plan_fast(const PaletteType* palette, typename PaletteType::mapped_pixel_type color, mixing_plan_data_fast* plan) {
                gfx_result rr ;
                if(nullptr==plan || nullptr==palette) {
                    return gfx_result::invalid_argument;
                }
                rgb_pixel<24> rgb888;
                rr = convert(color,&rgb888);
                if(gfx_result::success!=rr) {
                    return rr;
                }
                const unsigned r= rgb888.template channel<channel_name::R>(), 
                            g=rgb888.template channel<channel_name::G>(), 
                            b=rgb888.template channel<channel_name::B>();

                *plan = { {0,0}, 0.5 };
                double least_penalty = INFINITY;
                for(unsigned index1 = 0; index1 < PaletteType::size; ++index1)
                for(unsigned index2 = index1; index2 < PaletteType::size; ++index2)
                {
                    // Determine the two component colors
                    typename PaletteType::mapped_pixel_type mpx1;
                    rr=palette->map(typename PaletteType::pixel_type(index1),&mpx1);
                    if(gfx_result::success!=rr) {
                        return rr;
                    }
                    typename PaletteType::mapped_pixel_type mpx2;
                    rr=palette->map(typename PaletteType::pixel_type(index2),&mpx1);
                    if(gfx_result::success!=rr) {
                        return rr;
                    }
                    rr = convert(mpx1,&rgb888);
                    if(gfx_result::success!=rr) {
                        return rr;
                    }   
                    unsigned r1= rgb888.template channel<channel_name::R>(), 
                            g1=rgb888.template channel<channel_name::G>(), 
                            b1=rgb888.template channel<channel_name::B>();
                    rr = convert(mpx2,&rgb888);
                    if(gfx_result::success!=rr) {
                        return rr;
                    }
                    unsigned r2= rgb888.template channel<channel_name::R>(), 
                            g2=rgb888.template channel<channel_name::G>(), 
                            b2=rgb888.template channel<channel_name::B>();
                    int ratio = 32;
                    if(mpx1.native_value != mpx2.native_value)
                    {
                        // Determine the ratio of mixing for each channel.
                        //   solve(r1 + ratio*(r2-r1)/64 = r, ratio)
                        // Take a weighed average of these three ratios according to the
                        // perceived luminosity of each channel (according to CCIR 601).
                        ratio = ((r2 != r1 ? 299*64 * int(r - r1) / int(r2-r1) : 0)
                            +  (g2 != g1 ? 587*64 * int(g - g1) / int(g2-g1) : 0)
                            +  (b1 != b2 ? 114*64 * int(b - b1) / int(b2-b1) : 0))
                            / ((r2 != r1 ? 299 : 0)
                            + (g2 != g1 ? 587 : 0)
                            + (b2 != b1 ? 114 : 0));
                        if(ratio < 0) ratio = 0; else if(ratio > 63) ratio = 63;   
                    }
                    // Determine what mixing them in this proportion will produce
                    unsigned r0 = r1 + ratio * int(r2-r1) / 64;
                    unsigned g0 = g1 + ratio * int(g2-g1) / 64;
                    unsigned b0 = b1 + ratio * int(b2-b1) / 64;
                    double penalty = mixing_error(
                        r,g,b, r0,g0,b0, r1,g1,b1, r2,g2,b2,
                        ratio / double(64));
                    if(penalty < least_penalty)
                    {
                        least_penalty = penalty;
                        plan->colors[0] = index1;
                        plan->colors[1] = index2;
                        plan->ratio = ratio / double(64);
                    }
                }
                
                return gfx_result::success;
            }
            template<typename PaletteType>
            static gfx_result mixing_plan(const PaletteType* palette, typename PaletteType::mapped_pixel_type color, typename PaletteType::pixel_type* plan,double error_multiplier = 0.9 ) {
                gfx_result r ;
                if(nullptr==plan || nullptr==palette) {
                    return gfx_result::invalid_argument;
                }
                rgb_pixel<24> rgb888;
                r = convert(color,&rgb888);
                if(gfx_result::success!=r) {
                    return r;
                }
                const int src[3] = { rgb888.template channel<channel_name::R>(), rgb888.template channel<channel_name::G>(), rgb888.template channel<channel_name::B>() };

                int e[3] = { 0, 0, 0 }; // Error accumulator
                for(unsigned c=0; c<64; ++c)
                {
                    // Current temporary value
                    int t[3] = { (int)(src[0] + e[0] * error_multiplier), (int)(src[1] + e[1] * error_multiplier), (int)(src[2] + e[2] * error_multiplier) };
                    // Clamp it in the allowed RGB range
                    t[0]=helpers::clamp(t[0],0,255);
                    t[1]=helpers::clamp(t[1],0,255);
                    t[2]=helpers::clamp(t[2],0,255);
                    // Find the closest color from the palette
                    double least_penalty = INFINITY;
                    unsigned chosen = c%16;
                    for(unsigned index=0; index<PaletteType::size; ++index)
                    {
                        typename PaletteType::pixel_type ipx(index);
                        typename PaletteType::mapped_pixel_type col;
                        r= palette->map(ipx,&col);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        r=convert(col,&rgb888);
                        if(gfx_result::success!=r) {
                            return r;
                        }
                        const int pc[3] = { rgb888.template channel<channel_name::R>(), rgb888.template channel<channel_name::G>(), rgb888.template channel<channel_name::B>() };
                        double penalty = color_compare(pc[0],pc[1],pc[2], t[0],t[1],t[2]);
                        if(penalty < least_penalty)
                            { least_penalty = penalty; chosen=index; }
                    }
                    // Add it to candidates and update the error
                    plan[c] = typename PaletteType::pixel_type(chosen);
                    typename PaletteType::mapped_pixel_type mcolor;
                    r=palette->map(plan[c],&mcolor);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    r=convert(mcolor,&rgb888);
                    if(gfx_result::success!=r) {
                        return r;
                    }
                    const int pc[3] = { rgb888.template channel<channel_name::R>(), rgb888.template channel<channel_name::G>(), rgb888.template channel<channel_name::B>() };
                    e[0] += src[0]-pc[0];
                    e[1] += src[1]-pc[1];
                    e[2] += src[2]-pc[2];
                }
                // Sort the colors according to luminance
                qsort(plan,64,sizeof(typename PaletteType::pixel_type),[](const void* a,const void*b){
                    
                    return (int)(((unsigned*)dither_luminosity_cache)[((const typename PaletteType::pixel_type*)b)->template channel<0>()]-
                        ((unsigned*)dither_luminosity_cache)[((const typename PaletteType::pixel_type*)a)->template channel<0>()]);
                });
                return gfx_result::success;
            }
        };
    }
}
#endif