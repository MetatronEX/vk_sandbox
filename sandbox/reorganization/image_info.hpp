#ifndef IMAGE_INFO_HPP
#define IMAGE_INFO_HPP

struct image_info
{
    uint32_t        image_width;
    uint32_t        image_height;
    uint32_t        image_mip_level {1};
    uint32_t        image_layer_count {1};
    size_t          image_size;
    uint8_t*        image_binary {nullptr};
};

#endif