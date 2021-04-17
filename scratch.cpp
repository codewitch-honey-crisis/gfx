#define HTCW_LITTLE_ENDIAN
#include <stdio.h>
#include "include/bits.hpp"
template<size_t BitDepth,size_t BitsToLeft, size_t BitsToRight, size_t BitsPadRight> 
struct channel {
    using int_type = bits::uintx<bits::get_word_size(BitDepth)>;
    static const int_type int_mask = ~int_type(0);
    static const int_type mask =int_type(int_mask>>((sizeof(int_type)*8)-BitDepth));
    static const size_t bit_depth = BitDepth;
    static const size_t bits_to_left = BitsToLeft;
    static const size_t bits_to_right = BitsToRight;
    static const size_t pad_right_bits = BitsPadRight;
    static const size_t total_bits_to_right = bits_to_right+pad_right_bits;
    
};
int main() {
    using ch0 = channel<21,0,43,0>;
    using ch1 = channel<22,21,21,0>;
    using ch2 = channel<21,43,0,0>;
    
    //using ch0 = channel<5,0,11,0>;
    //using ch1 = channel<6,5,5,0>;
    //using ch2 = channel<5,11,0,0>;
    
    const size_t bit_depth = ch0::bit_depth+ch1::bit_depth+ch2::bit_depth;
    using int_type = bits::uintx<bits::get_word_size(bit_depth)>;

    using ch = ch1;

    int_type pixel = int_type(~int_type(0));

    ch::int_type val = 0xC;

    printf("initial channel value: %d (0x%X)\r\n",(int)val,(int)val);

    ch::int_type val_mask = ch::mask;

    int_type channel_mask = int_type(val_mask)<<ch::total_bits_to_right;
    pixel = int_type(pixel & int_type(~channel_mask))|int_type(int_type(val & ch::mask)<<ch::total_bits_to_right);
    
    val = 0;

    val = (pixel>>ch::total_bits_to_right)&ch::mask;

    printf("final channel value: %d (0x%X)\r\n",(int)val,(int)val);

    printf("pixel byte order: ");
    for(size_t i = 0;i<sizeof(pixel);++i) {
        printf("%02X",(int)((uint8_t*)&pixel)[i]);
    }
    printf("\r\n");
    return 0;
}