#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "image_processor.h"

void print_help() {
    std::cout << "Uso: ./image_processor entrada.jpg salida.jpg [opciones]\n";
    std::cout << "Opciones:\n";
    std::cout << "  -angulo <grados>    Rotar la imagen (ej. -angulo 45)\n";
    std::cout << "  -escalar <factor>   Escalar la imagen (ej. -escalar 1.5)\n";
    std::cout << "  -buddy              Usar Buddy System para gestión de memoria\n";
    std::cout << "  -help               Mostrar esta ayuda\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_help();
        return 1;
    }
    
    std::string input_file = argv[1];
    std::string output_file = argv[2];
    
    double rotate_angle = 0.0;
    double scale_factor = 1.0;
    bool use_buddy = false;
    
    // Procesar argumentos
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-angulo" && i + 1 < argc) {
            rotate_angle = std::stod(argv[++i]);
        } else if (arg == "-escalar" && i + 1 < argc) {
            scale_factor = std::stod(argv[++i]);
        } else if (arg == "-buddy") {
            use_buddy = true;
        } else if (arg == "-help") {
            print_help();
            return 0;
        } else {
            std::cerr << "Opción desconocida: " << arg << std::endl;
            print_help();
            return 1;
        }
    }
    
    // Procesar la imagen
    try {
        ImageProcessor processor;
        auto load_start = std::chrono::high_resolution_clock::now();
        
        // Cargar la imagen principal (DESCOMENTADO)
        if (!processor.load_image(input_file, use_buddy)) {
            std::cerr << "Error al cargar la imagen" << std::endl;
            return 1;
        }
        
        auto load_end = std::chrono::high_resolution_clock::now();
        
        // Solo comparar rendimiento si se usa Buddy System
        if (use_buddy) {
            std::cout << "\nEjecutando pruebas de rendimiento comparativo..." << std::endl;
            
            try {
                // Ejecutar con Buddy System
                ImageProcessor buddy_processor;
                if (!buddy_processor.load_image(input_file, true)) {
                    throw std::runtime_error("Error al cargar imagen para prueba Buddy System");
                }
                buddy_processor.compare_performance(true);
                
                // Ejecutar con asignación convencional
                ImageProcessor std_processor;
                if (!std_processor.load_image(input_file, false)) {
                    throw std::runtime_error("Error al cargar imagen para prueba convencional");
                }
                std_processor.compare_performance(false);
            } catch (const std::exception& e) {
                std::cerr << "Error en pruebas comparativas: " << e.what() << std::endl;
                return 1;
            }
        }

        processor.print_info();
        
        // Rotar si es necesario
        if (rotate_angle != 0.0) {
            std::cout << "\nRotando imagen " << rotate_angle << " grados..." << std::endl;
            processor.rotate(rotate_angle);
        }
        
        // Escalar si es necesario
        if (scale_factor != 1.0) {
            std::cout << "\nEscalando imagen con factor " << scale_factor << "..." << std::endl;
            processor.scale(scale_factor);
            processor.print_info(); // Mostrar nuevas dimensiones
        }
        
        // Guardar la imagen
        std::cout << "\nGuardando imagen procesada..." << std::endl;
        auto save_start = std::chrono::high_resolution_clock::now();
        if (!processor.save_image(output_file)) {
            std::cerr << "Error al guardar la imagen" << std::endl;
            return 1;
        }
        auto save_end = std::chrono::high_resolution_clock::now();
        
        // Mostrar tiempos
        auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_start);
        auto save_time = std::chrono::duration_cast<std::chrono::milliseconds>(save_end - save_start);
        
        std::cout << "\n=== Resumen de procesamiento ===" << std::endl;
        std::cout << "Tiempo de carga: " << load_time.count() << " ms" << std::endl;
        if (rotate_angle != 0.0) {
            std::cout << "Ángulo de rotación: " << rotate_angle << " grados" << std::endl;
        }
        if (scale_factor != 1.0) {
            std::cout << "Factor de escalado: " << scale_factor << std::endl;
        }
        std::cout << "Tiempo de guardado: " << save_time.count() << " ms" << std::endl;
        std::cout << "Modo de memoria: " << (use_buddy ? "Buddy System" : "Convencional (new/delete)") << std::endl;
        std::cout << "=================================" << std::endl;
        
        std::cout << "\nImagen procesada guardada exitosamente como: " << output_file << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}