/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== voxline_logo_mark_light.svg ==================
static const unsigned char temp_binary_data_0[] =
"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"256\" height=\"256\" viewBox=\"0 0 256 256\"><defs><linearGradient id=\"g\" x1=\"38\" y1=\"42\" x2=\"218\" y2=\"214\" gradientUnits=\"userSpaceOnUse\"><stop stop-color=\"#D86F96\"/><stop offset=\"1\" "
"stop-color=\"#E99A5C\"/></linearGradient><filter id=\"shadow\" x=\"-20%\" y=\"-20%\" width=\"140%\" height=\"140%\"><feDropShadow dx=\"0\" dy=\"14\" stdDeviation=\"16\" flood-color=\"#D86F96\" flood-opacity=\".28\"/></filter></defs><rect x=\"28\" y=\""
"28\" width=\"200\" height=\"200\" rx=\"52\" fill=\"#F7F0E7\" stroke=\"#DDD2C4\" stroke-width=\"4\"/><path d=\"M65 83 L106 177 C110 186 123 186 127 177 L148 126 C152 116 166 116 170 126 L191 176\" fill=\"none\" stroke=\"url(#g)\" stroke-width=\"18\" s"
"troke-linecap=\"round\" stroke-linejoin=\"round\" filter=\"url(#shadow)\"/><path d=\"M79 123 C99 98 121 98 141 123 C161 148 183 148 205 123\" fill=\"none\" stroke=\"#B8A6F3\" stroke-width=\"8\" stroke-linecap=\"round\" opacity=\".7\"/></svg>";

const char* voxline_logo_mark_light_svg = (const char*) temp_binary_data_0;

}

#include "BinaryData.h"

namespace BinaryData
{

const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x472c20f1:  numBytes = 904; return voxline_logo_mark_light_svg;
        case 0xa293ba25:  numBytes = 904; return voxline_logo_mark_dark_svg;
        case 0xf9b0ce9f:  numBytes = 343; return settings_light_svg;
        case 0x7f0b9eb7:  numBytes = 343; return settings_dark_svg;
        case 0x2ae80d24:  numBytes = 331; return bypass_light_svg;
        case 0x785ffb92:  numBytes = 331; return bypass_dark_svg;
        case 0x62086343:  numBytes = 487; return listen_light_svg;
        case 0x50dce593:  numBytes = 487; return listen_dark_svg;
        case 0x4f78b820:  numBytes = 320; return subtle_noise_texture_light_svg;
        case 0x81d00116:  numBytes = 320; return subtle_noise_texture_dark_svg;
        case 0x5d6e4754:  numBytes = 393; return soft_radial_glow_light_svg;
        case 0xbc11bb62:  numBytes = 393; return soft_radial_glow_dark_svg;
        case 0xee58c026:  numBytes = 237; return moon_svg;
        case 0x914ea5f1:  numBytes = 353; return sun_svg;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "voxline_logo_mark_light_svg",
    "voxline_logo_mark_dark_svg",
    "settings_light_svg",
    "settings_dark_svg",
    "bypass_light_svg",
    "bypass_dark_svg",
    "listen_light_svg",
    "listen_dark_svg",
    "subtle_noise_texture_light_svg",
    "subtle_noise_texture_dark_svg",
    "soft_radial_glow_light_svg",
    "soft_radial_glow_dark_svg",
    "moon_svg",
    "sun_svg"
};

const char* originalFilenames[] =
{
    "voxline_logo_mark_light.svg",
    "voxline_logo_mark_dark.svg",
    "settings_light.svg",
    "settings_dark.svg",
    "bypass_light.svg",
    "bypass_dark.svg",
    "listen_light.svg",
    "listen_dark.svg",
    "subtle_noise_texture_light.svg",
    "subtle_noise_texture_dark.svg",
    "soft_radial_glow_light.svg",
    "soft_radial_glow_dark.svg",
    "moon.svg",
    "sun.svg"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
