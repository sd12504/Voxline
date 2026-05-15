#include "Assets.h"

namespace
{
    juce::Image rasterize(const char* data, int size, int w, int h)
    {
        if (data == nullptr || size <= 0)
            return {};

        auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(data, size));
        if (!xml)
            return {};

        auto drawable = juce::Drawable::createFromSVG(*xml);
        if (!drawable)
            return {};

        juce::Image img(juce::Image::ARGB, w, h, true);
        juce::Graphics g(img);
        drawable->drawWithin(g, img.getBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
        return img;
    }
}

namespace VoxlineAssets
{

juce::Image loadLogoMark(bool dark, int targetSize)
{
    if (dark)
        return rasterize(BinaryData::voxline_logo_mark_dark_svg,
                         BinaryData::voxline_logo_mark_dark_svgSize,
                         targetSize, targetSize);
    return rasterize(BinaryData::voxline_logo_mark_light_svg,
                     BinaryData::voxline_logo_mark_light_svgSize,
                     targetSize, targetSize);
}

juce::Image loadSettingsIcon(bool dark, int targetSize)
{
    if (dark)
        return rasterize(BinaryData::settings_dark_svg,
                         BinaryData::settings_dark_svgSize,
                         targetSize, targetSize);
    return rasterize(BinaryData::settings_light_svg,
                     BinaryData::settings_light_svgSize,
                     targetSize, targetSize);
}

juce::Image loadBypassIcon(bool dark, int targetSize)
{
    if (dark)
        return rasterize(BinaryData::bypass_dark_svg,
                         BinaryData::bypass_dark_svgSize,
                         targetSize, targetSize);
    return rasterize(BinaryData::bypass_light_svg,
                     BinaryData::bypass_light_svgSize,
                     targetSize, targetSize);
}

juce::Image loadListenIcon(bool dark, int targetSize)
{
    if (dark)
        return rasterize(BinaryData::listen_dark_svg,
                         BinaryData::listen_dark_svgSize,
                         targetSize, targetSize);
    return rasterize(BinaryData::listen_light_svg,
                     BinaryData::listen_light_svgSize,
                     targetSize, targetSize);
}

juce::Image loadNoiseTexture(bool dark, int targetW, int targetH)
{
    if (dark)
        return rasterize(BinaryData::subtle_noise_texture_dark_svg,
                         BinaryData::subtle_noise_texture_dark_svgSize,
                         targetW, targetH);
    return rasterize(BinaryData::subtle_noise_texture_light_svg,
                     BinaryData::subtle_noise_texture_light_svgSize,
                     targetW, targetH);
}

juce::Image loadRadialGlow(bool dark, int targetW, int targetH)
{
    if (dark)
        return rasterize(BinaryData::soft_radial_glow_dark_svg,
                         BinaryData::soft_radial_glow_dark_svgSize,
                         targetW, targetH);
    return rasterize(BinaryData::soft_radial_glow_light_svg,
                     BinaryData::soft_radial_glow_light_svgSize,
                     targetW, targetH);
}

}
