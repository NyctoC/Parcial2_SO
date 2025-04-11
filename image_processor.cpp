#include "image_processor.h"
#include <cmath>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <cstring>
#include "stb_image.h"
#include "stb_image_write.h"

//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION

ImageProcessor::ImageProcessor() : width(0), height(0), channels(0), pixels(nullptr), using_buddy(false) {}

ImageProcessor::~ImageProcessor() {
    if (pixels) {
        free_pixels(pixels, height, using_buddy);
    }
}

ImageProcessor::Pixel** ImageProcessor::allocate_pixels(int w, int h, bool use_buddy) {
    if (use_buddy) {
        // Calcular el tamaño total necesario
        size_t total_size = h * sizeof(Pixel*) + h * w * sizeof(Pixel);
        buddy_allocator = std::make_unique<BuddyAllocator>(total_size * 2); // Asignar el doble para tener margen
        
        // Asignar memoria para los punteros a filas
        Pixel** rows = static_cast<Pixel**>(buddy_allocator->allocate(h * sizeof(Pixel*)));
        if (!rows) {
            throw std::bad_alloc();
        }
        
        // Asignar memoria para todos los píxeles en un solo bloque
        Pixel* pixel_block = static_cast<Pixel*>(buddy_allocator->allocate(h * w * sizeof(Pixel)));
        if (!pixel_block) {
            throw std::bad_alloc();
        }
        
        // Configurar los punteros a filas
        for (int y = 0; y < h; ++y) {
            rows[y] = pixel_block + y * w;
        }
        
        return rows;
    } else {
        // Asignación convencional
        Pixel** rows = new Pixel*[h];
        for (int y = 0; y < h; ++y) {
            rows[y] = new Pixel[w];
        }
        return rows;
    }
}

void ImageProcessor::free_pixels(Pixel** ptr, int h, bool use_buddy) {
    if (use_buddy) {
        // Buddy System maneja la liberación automáticamente en su destructor
        buddy_allocator.reset();
    } else {
        // Liberación convencional
        for (int y = 0; y < h; ++y) {
            delete[] ptr[y];
        }
        delete[] ptr;
    }
}

bool ImageProcessor::load_image(const std::string& filename, bool use_buddy) {
    if (pixels) {
        free_pixels(pixels, height, using_buddy);
        pixels = nullptr;
    }

    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Error al cargar la imagen: " << stbi_failure_reason() << std::endl;
        return false;
    }

    using_buddy = use_buddy;
    
    try {
        pixels = allocate_pixels(width, height, use_buddy);
        
        // Copiar datos
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = (y * width + x) * channels;
                pixels[y][x].r = data[index];
                pixels[y][x].g = data[index + 1];
                pixels[y][x].b = data[index + 2];
                pixels[y][x].a = (channels == 4) ? data[index + 3] : 255;
            }
        }
        
        stbi_image_free(data);
        return true;
    } catch (...) {
        stbi_image_free(data);
        throw;
    }
}

bool ImageProcessor::save_image(const std::string& filename) const {
    if (!pixels) return false;
    
    // Convertir nuestra estructura a un buffer plano
    unsigned char* data = new unsigned char[width * height * channels];
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;
            data[index] = pixels[y][x].r;
            data[index + 1] = pixels[y][x].g;
            data[index + 2] = pixels[y][x].b;
            if (channels == 4) {
                data[index + 3] = pixels[y][x].a;
            }
        }
    }
    
    // Guardar la imagen
    bool success = false;
    if (filename.substr(filename.find_last_of(".") + 1) == "png") {
        success = stbi_write_png(filename.c_str(), width, height, channels, data, width * channels);
    } else if (filename.substr(filename.find_last_of(".") + 1) == "jpg" || 
               filename.substr(filename.find_last_of(".") + 1) == "jpeg") {
        success = stbi_write_jpg(filename.c_str(), width, height, channels, data, 90);
    } else {
        std::cerr << "Formato de archivo no soportado" << std::endl;
    }
    
    delete[] data;
    return success;
}

ImageProcessor::Pixel ImageProcessor::interpolate(double x, double y) const {
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    // Asegurarnos de que estamos dentro de los límites
    x0 = std::max(0, std::min(width - 1, x0));
    y0 = std::max(0, std::min(height - 1, y0));
    x1 = std::max(0, std::min(width - 1, x1));
    y1 = std::max(0, std::min(height - 1, y1));
    
    double dx = x - x0;
    double dy = y - y0;
    
    Pixel p00 = get_pixel(x0, y0);
    Pixel p01 = get_pixel(x0, y1);
    Pixel p10 = get_pixel(x1, y0);
    Pixel p11 = get_pixel(x1, y1);
    
    // Interpolación bilineal
    Pixel result;
    result.r = static_cast<unsigned char>(
        p00.r * (1 - dx) * (1 - dy) + 
        p10.r * dx * (1 - dy) + 
        p01.r * (1 - dx) * dy + 
        p11.r * dx * dy
    );
    
    result.g = static_cast<unsigned char>(
        p00.g * (1 - dx) * (1 - dy) + 
        p10.g * dx * (1 - dy) + 
        p01.g * (1 - dx) * dy + 
        p11.g * dx * dy
    );
    
    result.b = static_cast<unsigned char>(
        p00.b * (1 - dx) * (1 - dy) + 
        p10.b * dx * (1 - dy) + 
        p01.b * (1 - dx) * dy + 
        p11.b * dx * dy
    );
    
    result.a = static_cast<unsigned char>(
        p00.a * (1 - dx) * (1 - dy) + 
        p10.a * dx * (1 - dy) + 
        p01.a * (1 - dx) * dy + 
        p11.a * dx * dy
    );
    
    return result;
}

ImageProcessor::Pixel ImageProcessor::get_pixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return Pixel{0, 0, 0, 0}; // Pixel negro para coordenadas fuera de límites
    }
    return pixels[y][x];
}

void ImageProcessor::set_pixel(int x, int y, const Pixel& pixel) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        pixels[y][x] = pixel;
    }
}

void ImageProcessor::rotate(double angle, unsigned char fill_r, unsigned char fill_g, 
                           unsigned char fill_b, unsigned char fill_a) {
    if (!pixels) return;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    rotate_internal(angle, fill_r, fill_g, fill_b, fill_a);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Rotación completada en " << duration.count() << " ms" << std::endl;
}

void ImageProcessor::rotate_internal(double angle, unsigned char fill_r, unsigned char fill_g, 
                                    unsigned char fill_b, unsigned char fill_a) {
    // Convertir ángulo a radianes
    double radians = angle * M_PI / 180.0;
    
    // Calcular centro de la imagen
    double center_x = width / 2.0;
    double center_y = height / 2.0;
    
    // Crear una nueva imagen rotada (mismo tamaño)
    Pixel** rotated = allocate_pixels(width, height, using_buddy);
    
    // Rellenar con el color de fondo
    Pixel fill_pixel{fill_r, fill_g, fill_b, fill_a};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rotated[y][x] = fill_pixel;
        }
    }
    
    // Aplicar rotación
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Convertir a coordenadas relativas al centro
            double rel_x = x - center_x;
            double rel_y = y - center_y;
            
            // Aplicar rotación inversa
            double src_x = center_x + rel_x * cos(radians) + (rel_y * sin(radians));
            double src_y = center_y + -rel_x * sin(radians) + (rel_y * cos(radians));
            
            // Si el punto de origen está dentro de la imagen original, interpolar
            if (src_x >= 0 && src_x < width - 1 && src_y >= 0 && src_y < height - 1) {
                rotated[y][x] = interpolate(src_x, src_y);
            }
        }
    }
    
    // Liberar la imagen original y reemplazar con la rotada
    free_pixels(pixels, height, using_buddy);
    pixels = rotated;
}

void ImageProcessor::scale(double factor) {
    if (!pixels || factor <= 0) return;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Guardar dimensiones originales
    int old_width = width;
    int old_height = height;
    
    scale_internal(factor);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Mostrar información del escalado
    std::cout << "\n=== Información de Escalado ===" << std::endl;
    std::cout << "Dimensiones originales: " << old_width << " x " << old_height << std::endl;
    std::cout << "Factor de escalado: " << factor << std::endl;
    std::cout << "Nuevas dimensiones: " << width << " x " << height << std::endl;
    std::cout << "Tiempo de escalado: " << duration.count() << " ms" << std::endl;
    std::cout << "=============================" << std::endl;
}

void ImageProcessor::scale_internal(double factor) {
    int new_width = static_cast<int>(width * factor);
    int new_height = static_cast<int>(height * factor);
    
    // Crear nueva imagen escalada
    Pixel** scaled = allocate_pixels(new_width, new_height, using_buddy);
    
    // Escalar la imagen
    for (int y = 0; y < new_height; ++y) {
        for (int x = 0; x < new_width; ++x) {
            // Mapear coordenadas de la nueva imagen a la original
            double src_x = (x + 0.5) / factor - 0.5;
            double src_y = (y + 0.5) / factor - 0.5;
            
            // Interpolar el valor del píxel
            scaled[y][x] = interpolate(src_x, src_y);
        }
    }
    
    // Actualizar dimensiones
    int old_height = height;
    width = new_width;
    height = new_height;
    
    // Liberar la imagen original y reemplazar con la escalada
    free_pixels(pixels, old_height, using_buddy);
    pixels = scaled;
}

ImageProcessor::MemoryUsage ImageProcessor::get_memory_usage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    MemoryUsage mem;
    mem.memory_used = usage.ru_maxrss; // KB
    mem.peak_memory = usage.ru_ixrss;  // KB
    return mem;
}

void ImageProcessor::compare_performance(bool use_buddy) {
    // Medir memoria inicial
    MemoryUsage mem_before = get_memory_usage();
    
    // Medir tiempo de rotación
    auto rotate_start = std::chrono::high_resolution_clock::now();
    rotate_internal(45.0, 0, 0, 0, 255); // Rotación de 45 grados con fondo negro
    auto rotate_end = std::chrono::high_resolution_clock::now();
    
    // Medir tiempo de escalado
    auto scale_start = std::chrono::high_resolution_clock::now();
    scale_internal(1.5); // Escalado de 1.5x
    auto scale_end = std::chrono::high_resolution_clock::now();
    
    // Medir memoria final
    MemoryUsage mem_after = get_memory_usage();
    
    // Calcular diferencias
    auto rotate_time = std::chrono::duration_cast<std::chrono::milliseconds>(rotate_end - rotate_start);
    auto scale_time = std::chrono::duration_cast<std::chrono::milliseconds>(scale_end - scale_start);
    size_t memory_used = mem_after.memory_used - mem_before.memory_used;
    
    // Mostrar resultados
    std::cout << "\n=== COMPARACIÓN DE RENDIMIENTO ===" << std::endl;
    std::cout << "Modo de memoria: " << (use_buddy ? "Buddy System" : "Convencional (new/delete)") << std::endl;
    std::cout << "Tiempo de rotación: " << rotate_time.count() << " ms" << std::endl;
    std::cout << "Tiempo de escalado: " << scale_time.count() << " ms" << std::endl;
    std::cout << "Memoria utilizada: " << memory_used << " KB" << std::endl;
    std::cout << "Memoria máxima: " << mem_after.peak_memory << " KB" << std::endl;
    std::cout << "=================================" << std::endl;
}

void ImageProcessor::print_info() const {
    std::cout << "\n=== Información de la Imagen ===" << std::endl;
    std::cout << "Archivo cargado" << std::endl;
    std::cout << "Dimensiones: " << width << " x " << height << " px" << std::endl;
    std::cout << "Canales: " << channels << " (" << (channels == 3 ? "RGB" : "RGBA") << ")" << std::endl;
    std::cout << "Gestión de memoria: " << (using_buddy ? "Buddy System" : "new/delete") << std::endl;
    std::cout << "===============================" << std::endl;
}