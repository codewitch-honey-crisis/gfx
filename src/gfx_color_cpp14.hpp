#ifndef HTCW_GFX_COLOR_CPP14_HPP
#define HTCW_GFX_COLOR_CPP14_HPP
#include "gfx_pixel.hpp"
namespace gfx {
        // predefined color values
        template<typename PixelType>
        struct color final {
                // we use a super precise max-bit RGB pixel for this
                using source_type = rgb_pixel<HTCW_MAX_WORD>;
                static const PixelType alice_blue;
                static const PixelType antique_white;
                static const PixelType aqua;
                static const PixelType aquamarine;
                static const PixelType azure;
                static const PixelType beige;
                static const PixelType bisque;
                static const PixelType black;
                static const PixelType blanched_almond;
                static const PixelType blue;
                static const PixelType blue_violet;
                static const PixelType brown;
                static const PixelType burly_wood;
                static const PixelType cadet_blue;
                static const PixelType chartreuse;
                static const PixelType chocolate;
                static const PixelType coral;
                static const PixelType cornflower_blue;
                static const PixelType cornsilk;
                static const PixelType crimson;
                static const PixelType cyan;
                static const PixelType dark_blue;
                static const PixelType dark_cyan;
                static const PixelType dark_goldenrod;
                static const PixelType dark_gray;
                static const PixelType dark_green;
                static const PixelType dark_khaki;
                static const PixelType dark_magenta;
                static const PixelType dark_olive_green;
                static const PixelType dark_orange;
                static const PixelType dark_orchid;
                static const PixelType dark_red;
                static const PixelType dark_salmon;
                static const PixelType dark_sea_green;
                static const PixelType dark_steel_blue;
                static const PixelType dark_slate_gray;
                static const PixelType dark_turquoise;
                static const PixelType dark_violet;
                static const PixelType deep_pink;
                static const PixelType deep_sky_blue;
                static const PixelType dim_gray;
                static const PixelType dodger_blue;
                static const PixelType firebrick;
                static const PixelType floral_white;
                static const PixelType forest_green;
                static const PixelType fuchsia;
                static const PixelType gainsboro;
                static const PixelType ghost_white;
                static const PixelType gold;
                static const PixelType goldenrod;
                static const PixelType gray;
                static const PixelType green;
                static const PixelType green_yellow;
                static const PixelType honeydew;
                static const PixelType hot_pink;
                static const PixelType indian_red;
                static const PixelType indigo;
                static const PixelType ivory;
                static const PixelType khaki;
                static const PixelType lavender;
                static const PixelType lavender_blush;
                static const PixelType lawn_green;
                static const PixelType lemon_chiffon;
                static const PixelType light_blue;
                static const PixelType light_coral;
                static const PixelType light_cyan;
                static const PixelType light_goldenrod_yellow;
                static const PixelType light_green;
                static const PixelType light_gray;
                static const PixelType light_pink;
                static const PixelType light_salmon;
                static const PixelType light_sea_green;
                static const PixelType light_sky_blue;
                static const PixelType light_slate_gray;
                static const PixelType light_steel_blue;
                static const PixelType light_yellow;
                static const PixelType lime;
                static const PixelType lime_green;
                static const PixelType linen;
                static const PixelType magenta;
                static const PixelType maroon;
                static const PixelType medium_aquamarine;
                static const PixelType medium_blue;
                static const PixelType medium_orchid;
                static const PixelType medium_purple;
                static const PixelType medium_sea_green;
                static const PixelType medium_steel_blue;
                static const PixelType medium_spring_green;
                static const PixelType medium_turquoise;
                static const PixelType medium_violet_red;
                static const PixelType midnight_blue;
                static const PixelType mint_cream;
                static const PixelType misty_rose;
                static const PixelType moccasin;
                static const PixelType navajo_white;
                static const PixelType navy;
                static const PixelType old_lace;
                static const PixelType olive;
                static const PixelType olive_drab;
                static const PixelType orange;
                static const PixelType orange_red;
                static const PixelType orchid;
                static const PixelType pale_goldenrod;
                static const PixelType pale_green;
                static const PixelType pale_turquoise;
                static const PixelType pale_violet_red;
                static const PixelType papaya_whip;
                static const PixelType peach_puff;
                static const PixelType peru;
                static const PixelType pink;
                static const PixelType plum;
                static const PixelType powder_blue;
                static const PixelType purple;
                static const PixelType red;
                static const PixelType rosy_brown;
                static const PixelType royal_blue;
                static const PixelType saddle_brown;
                static const PixelType salmon;
                static const PixelType sandy_brown;
                static const PixelType sea_green;
                static const PixelType sea_shell;
                static const PixelType sienna;
                static const PixelType silver;
                static const PixelType sky_blue;
                static const PixelType slate_blue;
                static const PixelType slate_gray;
                static const PixelType snow;
                static const PixelType spring_green;
                static const PixelType steel_blue;
                static const PixelType tan;
                static const PixelType teal;
                static const PixelType thistle;
                static const PixelType tomato;
                static const PixelType turquoise;
                static const PixelType violet;
                static const PixelType wheat;
                static const PixelType white;
                static const PixelType white_smoke;
                static const PixelType yellow;
                static const PixelType yellow_green;
};
        template<typename PixelType> const PixelType color<PixelType>::alice_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.941176470588235,0.972549019607843,1));
        template<typename PixelType> const PixelType color<PixelType>::antique_white = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.980392156862745,0.92156862745098,0.843137254901961));
        template<typename PixelType> const PixelType color<PixelType>::aqua = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,1,1));
        template<typename PixelType> const PixelType color<PixelType>::aquamarine = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.498039215686275,1,0.831372549019608));
        template<typename PixelType> const PixelType color<PixelType>::azure = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.941176470588235,1,1));
        template<typename PixelType> const PixelType color<PixelType>::beige = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.96078431372549,0.96078431372549,0.862745098039216));
        template<typename PixelType> const PixelType color<PixelType>::bisque = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.894117647058824,0.768627450980392));
        template<typename PixelType> const PixelType color<PixelType>::black = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0,0));
        template<typename PixelType> const PixelType color<PixelType>::blanched_almond = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.92156862745098,0.803921568627451));
        template<typename PixelType> const PixelType color<PixelType>::blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0,1));
        template<typename PixelType> const PixelType color<PixelType>::blue_violet = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.541176470588235,0.168627450980392,0.886274509803922));
        template<typename PixelType> const PixelType color<PixelType>::brown = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.647058823529412,0.164705882352941,0.164705882352941));
        template<typename PixelType> const PixelType color<PixelType>::burly_wood = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.870588235294118,0.72156862745098,0.529411764705882));
        template<typename PixelType> const PixelType color<PixelType>::cadet_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.372549019607843,0.619607843137255,0.627450980392157));
        template<typename PixelType> const PixelType color<PixelType>::chartreuse = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.498039215686275,1,0));
        template<typename PixelType> const PixelType color<PixelType>::chocolate = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.823529411764706,0.411764705882353,0.117647058823529));
        template<typename PixelType> const PixelType color<PixelType>::coral = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.498039215686275,0.313725490196078));
        template<typename PixelType> const PixelType color<PixelType>::cornflower_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.392156862745098,0.584313725490196,0.929411764705882));
        template<typename PixelType> const PixelType color<PixelType>::cornsilk = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.972549019607843,0.862745098039216));
        template<typename PixelType> const PixelType color<PixelType>::crimson = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.862745098039216,0.0784313725490196,0.235294117647059));
        template<typename PixelType> const PixelType color<PixelType>::cyan = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,1,1));
        template<typename PixelType> const PixelType color<PixelType>::dark_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0,0.545098039215686));
        template<typename PixelType> const PixelType color<PixelType>::dark_cyan = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.545098039215686,0.545098039215686));
        template<typename PixelType> const PixelType color<PixelType>::dark_goldenrod = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.72156862745098,0.525490196078431,0.0431372549019608));
        template<typename PixelType> const PixelType color<PixelType>::dark_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.662745098039216,0.662745098039216,0.662745098039216));
        template<typename PixelType> const PixelType color<PixelType>::dark_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.392156862745098,0));
        template<typename PixelType> const PixelType color<PixelType>::dark_khaki = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.741176470588235,0.717647058823529,0.419607843137255));
        template<typename PixelType> const PixelType color<PixelType>::dark_magenta = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.545098039215686,0,0.545098039215686));
        template<typename PixelType> const PixelType color<PixelType>::dark_olive_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.333333333333333,0.419607843137255,0.184313725490196));
        template<typename PixelType> const PixelType color<PixelType>::dark_orange = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.549019607843137,0));
        template<typename PixelType> const PixelType color<PixelType>::dark_orchid = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.6,0.196078431372549,0.8));
        template<typename PixelType> const PixelType color<PixelType>::dark_red = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.545098039215686,0,0));
        template<typename PixelType> const PixelType color<PixelType>::dark_salmon = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.913725490196078,0.588235294117647,0.47843137254902));
        template<typename PixelType> const PixelType color<PixelType>::dark_sea_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.56078431372549,0.737254901960784,0.545098039215686));
        template<typename PixelType> const PixelType color<PixelType>::dark_steel_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.282352941176471,0.23921568627451,0.545098039215686));
        template<typename PixelType> const PixelType color<PixelType>::dark_slate_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.184313725490196,0.309803921568627,0.309803921568627));
        template<typename PixelType> const PixelType color<PixelType>::dark_turquoise = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.807843137254902,0.819607843137255));
        template<typename PixelType> const PixelType color<PixelType>::dark_violet = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.580392156862745,0,0.827450980392157));
        template<typename PixelType> const PixelType color<PixelType>::deep_pink = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.0784313725490196,0.576470588235294));
        template<typename PixelType> const PixelType color<PixelType>::deep_sky_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.749019607843137,1));
        template<typename PixelType> const PixelType color<PixelType>::dim_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.411764705882353,0.411764705882353,0.411764705882353));
        template<typename PixelType> const PixelType color<PixelType>::dodger_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.117647058823529,0.564705882352941,1));
        template<typename PixelType> const PixelType color<PixelType>::firebrick = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.698039215686274,0.133333333333333,0.133333333333333));
        template<typename PixelType> const PixelType color<PixelType>::floral_white = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.980392156862745,0.941176470588235));
        template<typename PixelType> const PixelType color<PixelType>::forest_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.133333333333333,0.545098039215686,0.133333333333333));
        template<typename PixelType> const PixelType color<PixelType>::fuchsia = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0,1));
        template<typename PixelType> const PixelType color<PixelType>::gainsboro = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.862745098039216,0.862745098039216,0.862745098039216));
        template<typename PixelType> const PixelType color<PixelType>::ghost_white = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.972549019607843,0.972549019607843,1));
        template<typename PixelType> const PixelType color<PixelType>::gold = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.843137254901961,0));
        template<typename PixelType> const PixelType color<PixelType>::goldenrod = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.854901960784314,0.647058823529412,0.125490196078431));
        template<typename PixelType> const PixelType color<PixelType>::gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.501960784313725,0.501960784313725,0.501960784313725));
        template<typename PixelType> const PixelType color<PixelType>::green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.501960784313725,0));
        template<typename PixelType> const PixelType color<PixelType>::green_yellow = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.67843137254902,1,0.184313725490196));
        template<typename PixelType> const PixelType color<PixelType>::honeydew = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.941176470588235,1,0.941176470588235));
        template<typename PixelType> const PixelType color<PixelType>::hot_pink = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.411764705882353,0.705882352941177));
        template<typename PixelType> const PixelType color<PixelType>::indian_red = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.803921568627451,0.36078431372549,0.36078431372549));
        template<typename PixelType> const PixelType color<PixelType>::indigo = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.294117647058824,0,0.509803921568627));
        template<typename PixelType> const PixelType color<PixelType>::ivory = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,1,0.941176470588235));
        template<typename PixelType> const PixelType color<PixelType>::khaki = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.941176470588235,0.901960784313726,0.549019607843137));
        template<typename PixelType> const PixelType color<PixelType>::lavender = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.901960784313726,0.901960784313726,0.980392156862745));
        template<typename PixelType> const PixelType color<PixelType>::lavender_blush = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.941176470588235,0.96078431372549));
        template<typename PixelType> const PixelType color<PixelType>::lawn_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.486274509803922,0.988235294117647,0));
        template<typename PixelType> const PixelType color<PixelType>::lemon_chiffon = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.980392156862745,0.803921568627451));
        template<typename PixelType> const PixelType color<PixelType>::light_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.67843137254902,0.847058823529412,0.901960784313726));
        template<typename PixelType> const PixelType color<PixelType>::light_coral = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.941176470588235,0.501960784313725,0.501960784313725));
        template<typename PixelType> const PixelType color<PixelType>::light_cyan = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.87843137254902,1,1));
        template<typename PixelType> const PixelType color<PixelType>::light_goldenrod_yellow = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.980392156862745,0.980392156862745,0.823529411764706));
        template<typename PixelType> const PixelType color<PixelType>::light_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.564705882352941,0.933333333333333,0.564705882352941));
        template<typename PixelType> const PixelType color<PixelType>::light_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.827450980392157,0.827450980392157,0.827450980392157));
        template<typename PixelType> const PixelType color<PixelType>::light_pink = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.713725490196078,0.756862745098039));
        template<typename PixelType> const PixelType color<PixelType>::light_salmon = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.627450980392157,0.47843137254902));
        template<typename PixelType> const PixelType color<PixelType>::light_sea_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.125490196078431,0.698039215686274,0.666666666666667));
        template<typename PixelType> const PixelType color<PixelType>::light_sky_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.529411764705882,0.807843137254902,0.980392156862745));
        template<typename PixelType> const PixelType color<PixelType>::light_slate_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.466666666666667,0.533333333333333,0.6));
        template<typename PixelType> const PixelType color<PixelType>::light_steel_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.690196078431373,0.768627450980392,0.870588235294118));
        template<typename PixelType> const PixelType color<PixelType>::light_yellow = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,1,0.87843137254902));
        template<typename PixelType> const PixelType color<PixelType>::lime = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,1,0));
        template<typename PixelType> const PixelType color<PixelType>::lime_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.196078431372549,0.803921568627451,0.196078431372549));
        template<typename PixelType> const PixelType color<PixelType>::linen = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.980392156862745,0.941176470588235,0.901960784313726));
        template<typename PixelType> const PixelType color<PixelType>::magenta = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0,1));
        template<typename PixelType> const PixelType color<PixelType>::maroon = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.501960784313725,0,0));
        template<typename PixelType> const PixelType color<PixelType>::medium_aquamarine = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.4,0.803921568627451,0.666666666666667));
        template<typename PixelType> const PixelType color<PixelType>::medium_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0,0.803921568627451));
        template<typename PixelType> const PixelType color<PixelType>::medium_orchid = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.729411764705882,0.333333333333333,0.827450980392157));
        template<typename PixelType> const PixelType color<PixelType>::medium_purple = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.576470588235294,0.43921568627451,0.858823529411765));
        template<typename PixelType> const PixelType color<PixelType>::medium_sea_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.235294117647059,0.701960784313725,0.443137254901961));
        template<typename PixelType> const PixelType color<PixelType>::medium_steel_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.482352941176471,0.407843137254902,0.933333333333333));
        template<typename PixelType> const PixelType color<PixelType>::medium_spring_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.980392156862745,0.603921568627451));
        template<typename PixelType> const PixelType color<PixelType>::medium_turquoise = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.282352941176471,0.819607843137255,0.8));
        template<typename PixelType> const PixelType color<PixelType>::medium_violet_red = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.780392156862745,0.0823529411764706,0.52156862745098));
        template<typename PixelType> const PixelType color<PixelType>::midnight_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.0980392156862745,0.0980392156862745,0.43921568627451));
        template<typename PixelType> const PixelType color<PixelType>::mint_cream = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.96078431372549,1,0.980392156862745));
        template<typename PixelType> const PixelType color<PixelType>::misty_rose = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.894117647058824,0.882352941176471));
        template<typename PixelType> const PixelType color<PixelType>::moccasin = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.894117647058824,0.709803921568627));
        template<typename PixelType> const PixelType color<PixelType>::navajo_white = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.870588235294118,0.67843137254902));
        template<typename PixelType> const PixelType color<PixelType>::navy = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0,0.501960784313725));
        template<typename PixelType> const PixelType color<PixelType>::old_lace = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.992156862745098,0.96078431372549,0.901960784313726));
        template<typename PixelType> const PixelType color<PixelType>::olive = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.501960784313725,0.501960784313725,0));
        template<typename PixelType> const PixelType color<PixelType>::olive_drab = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.419607843137255,0.556862745098039,0.137254901960784));
        template<typename PixelType> const PixelType color<PixelType>::orange = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.647058823529412,0));
        template<typename PixelType> const PixelType color<PixelType>::orange_red = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.270588235294118,0));
        template<typename PixelType> const PixelType color<PixelType>::orchid = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.854901960784314,0.43921568627451,0.83921568627451));
        template<typename PixelType> const PixelType color<PixelType>::pale_goldenrod = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.933333333333333,0.909803921568627,0.666666666666667));
        template<typename PixelType> const PixelType color<PixelType>::pale_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.596078431372549,0.984313725490196,0.596078431372549));
        template<typename PixelType> const PixelType color<PixelType>::pale_turquoise = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.686274509803922,0.933333333333333,0.933333333333333));
        template<typename PixelType> const PixelType color<PixelType>::pale_violet_red = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.858823529411765,0.43921568627451,0.576470588235294));
        template<typename PixelType> const PixelType color<PixelType>::papaya_whip = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.937254901960784,0.835294117647059));
        template<typename PixelType> const PixelType color<PixelType>::peach_puff = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.854901960784314,0.725490196078431));
        template<typename PixelType> const PixelType color<PixelType>::peru = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.803921568627451,0.52156862745098,0.247058823529412));
        template<typename PixelType> const PixelType color<PixelType>::pink = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.752941176470588,0.796078431372549));
        template<typename PixelType> const PixelType color<PixelType>::plum = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.866666666666667,0.627450980392157,0.866666666666667));
        template<typename PixelType> const PixelType color<PixelType>::powder_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.690196078431373,0.87843137254902,0.901960784313726));
        template<typename PixelType> const PixelType color<PixelType>::purple = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.501960784313725,0,0.501960784313725));
        template<typename PixelType> const PixelType color<PixelType>::red = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0,0));
        template<typename PixelType> const PixelType color<PixelType>::rosy_brown = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.737254901960784,0.56078431372549,0.56078431372549));
        template<typename PixelType> const PixelType color<PixelType>::royal_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.254901960784314,0.411764705882353,0.882352941176471));
        template<typename PixelType> const PixelType color<PixelType>::saddle_brown = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.545098039215686,0.270588235294118,0.0745098039215686));
        template<typename PixelType> const PixelType color<PixelType>::salmon = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.980392156862745,0.501960784313725,0.447058823529412));
        template<typename PixelType> const PixelType color<PixelType>::sandy_brown = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.956862745098039,0.643137254901961,0.376470588235294));
        template<typename PixelType> const PixelType color<PixelType>::sea_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.180392156862745,0.545098039215686,0.341176470588235));
        template<typename PixelType> const PixelType color<PixelType>::sea_shell = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.96078431372549,0.933333333333333));
        template<typename PixelType> const PixelType color<PixelType>::sienna = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.627450980392157,0.32156862745098,0.176470588235294));
        template<typename PixelType> const PixelType color<PixelType>::silver = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.752941176470588,0.752941176470588,0.752941176470588));
        template<typename PixelType> const PixelType color<PixelType>::sky_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.529411764705882,0.807843137254902,0.92156862745098));
        template<typename PixelType> const PixelType color<PixelType>::slate_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.415686274509804,0.352941176470588,0.803921568627451));
        template<typename PixelType> const PixelType color<PixelType>::slate_gray = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.43921568627451,0.501960784313725,0.564705882352941));
        template<typename PixelType> const PixelType color<PixelType>::snow = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.980392156862745,0.980392156862745));
        template<typename PixelType> const PixelType color<PixelType>::spring_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,1,0.498039215686275));
        template<typename PixelType> const PixelType color<PixelType>::steel_blue = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.274509803921569,0.509803921568627,0.705882352941177));
        template<typename PixelType> const PixelType color<PixelType>::tan = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.823529411764706,0.705882352941177,0.549019607843137));
        template<typename PixelType> const PixelType color<PixelType>::teal = convert<source_type,PixelType>(color<PixelType>::source_type(true,0,0.501960784313725,0.501960784313725));
        template<typename PixelType> const PixelType color<PixelType>::thistle = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.847058823529412,0.749019607843137,0.847058823529412));
        template<typename PixelType> const PixelType color<PixelType>::tomato = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,0.388235294117647,0.27843137254902));
        template<typename PixelType> const PixelType color<PixelType>::turquoise = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.250980392156863,0.87843137254902,0.815686274509804));
        template<typename PixelType> const PixelType color<PixelType>::violet = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.933333333333333,0.509803921568627,0.933333333333333));
        template<typename PixelType> const PixelType color<PixelType>::wheat = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.96078431372549,0.870588235294118,0.701960784313725));
        template<typename PixelType> const PixelType color<PixelType>::white = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,1,1));
        template<typename PixelType> const PixelType color<PixelType>::white_smoke = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.96078431372549,0.96078431372549,0.96078431372549));
        template<typename PixelType> const PixelType color<PixelType>::yellow = convert<source_type,PixelType>(color<PixelType>::source_type(true,1,1,0));
        template<typename PixelType> const PixelType color<PixelType>::yellow_green = convert<source_type,PixelType>(color<PixelType>::source_type(true,0.603921568627451,0.803921568627451,0.196078431372549));

        using color_max = color<rgb_pixel<HTCW_MAX_WORD>>;
}
#endif