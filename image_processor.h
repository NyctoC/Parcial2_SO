#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <memory>
#include "buddy_allocator.h"
#include <sys/resource.h>

class ImageProcessor {
public:
    struct Pixel {
        unsigned char r, g, b, a;
    };
    
    ImageProcessor();
    ~ImageProcessor();
    
    bool load_image(const std::string& filename, bool use_buddy);
    bool save_image(const std::string& filename) const;
    
    void rotate(double angle, unsigned char fill_r = 0, unsigned char fill_g = 0, 
                unsigned char fill_b = 0, unsigned char fill_a = 255);
    void scale(double factor);
    
    int get_width() const { return width; }
    int get_height() const { return height; }
    int get_channels() const { return channels; }
    
    void print_info() const;

    struct MemoryUsage {
        size_t memory_used;
        size_t peak_memory;
    };
    
    static MemoryUsage get_memory_usage();
    void compare_performance(bool use_buddy);
    
private:
    int width, height, channels;
    Pixel** pixels;
    std::unique_ptr<BuddyAllocator> buddy_allocator;
    bool using_buddy;
    
    Pixel** allocate_pixels(int w, int h, bool use_buddy);
    void free_pixels(Pixel** ptr, int h, bool use_buddy);
    
    Pixel interpolate(double x, double y) const;
    Pixel get_pixel(int x, int y) const;
    void set_pixel(int x, int y, const Pixel& pixel);
    
    void rotate_internal(double angle, unsigned char fill_r, unsigned char fill_g, 
                        unsigned char fill_b, unsigned char fill_a);
    void scale_internal(double factor);
    
    // Deshabilitar copia
    ImageProcessor(const ImageProcessor&) = delete;
    ImageProcessor& operator=(const ImageProcessor&) = delete;
};

#endif