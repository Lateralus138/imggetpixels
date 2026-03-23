#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include "pch.h"
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <future>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "version.h"

using namespace argparser;

const char* HELP_HEADER = "Get colors and size information of an image file in\n"
                          "various formats. Values default to bare hexadecimal\n"
                          "format with transparency (e.g. FFFFFFFF). Supports\n"
                          "color matching against a palette and image output.";

void setup_parser(ArgumentParser& parser) {
    parser.add_switch_pair("v", "version", "\tShow the version of the utility.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("h", "help", "\tThis help message.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch("rgb", "\tOutput is in RGB with transparency.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch("BGR", "\tOutput is in BGR with transparency.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("o", "opaque", "\tOutput has no transparency values.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("p", "prefix", "\tPrepend a string to the output of hexadecimal values.", SwitchType::PARAMETER, Requirement::OPTIONAL);
    parser.add_switch_pair("c", "coordinates", "\tReturn the output in the format of x,y: <COLOR>.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("r", "resolution", "\tOnly return the width and height of the image.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("W", "width", "\tOnly return the image's width.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("H", "height", "\tOnly return the image's height.", SwitchType::FLAG, Requirement::OPTIONAL);
    parser.add_switch_pair("C", "closest-list", "\tGet the closest color of each pixel from a list of colors (delimited by ';').", SwitchType::PARAMETER, Requirement::OPTIONAL);
    parser.add_switch_pair("O", "output", "\tOutput an image from the retrieved colors.", SwitchType::PARAMETER, Requirement::OPTIONAL);
    parser.add_switch_pair("R", "resize", "\tResize the image after processing when outputting to file\n\t(format: <WIDTH>x<HEIGHT>).", SwitchType::PARAMETER, Requirement::OPTIONAL);
}

struct Color {
    unsigned char r, g, b, a;
    bool has_alpha;
};

struct Lab {
    double L, a, b;
};

struct PaletteColor {
    Color color;
    Lab lab;
};

struct OutputConfig {
    bool use_rgb;
    bool use_bgr;
    bool is_opaque;
    bool show_coords;
    std::string prefix;
    std::vector<PaletteColor> palette;
    std::string output_filename;
};

Lab rgb_to_lab(unsigned char r, unsigned char g, unsigned char b) {
    double R = r / 255.0;
    double G = g / 255.0;
    double B = b / 255.0;

    R = (R > 0.04045) ? std::pow((R + 0.055) / 1.055, 2.4) : (R / 12.92);
    G = (G > 0.04045) ? std::pow((G + 0.055) / 1.055, 2.4) : (G / 12.92);
    B = (B > 0.04045) ? std::pow((B + 0.055) / 1.055, 2.4) : (B / 12.92);

    R *= 100.0; G *= 100.0; B *= 100.0;

    double X = R * 0.4124 + G * 0.3576 + B * 0.1805;
    double Y = R * 0.2126 + G * 0.7152 + B * 0.0722;
    double Z = R * 0.0193 + G * 0.1192 + B * 0.9505;

    X /= 95.047;
    Y /= 100.000;
    Z /= 108.883;

    auto f = [](double t) {
        return (t > 0.008856) ? std::pow(t, 1.0/3.0) : (7.787 * t + 16.0/116.0);
    };

    double lab_L = 116.0 * f(Y) - 16.0;
    double lab_a = 500.0 * (f(X) - f(Y));
    double lab_b = 200.0 * (f(Y) - f(Z));

    return {lab_L, lab_a, lab_b};
}

double delta_e_2000(const Lab& lab1, const Lab& lab2) {
    const double kL = 1.0, kC = 1.0, kH = 1.0;
    double L1 = lab1.L, a1 = lab1.a, b1 = lab1.b;
    double L2 = lab2.L, a2 = lab2.a, b2 = lab2.b;

    double C1 = std::sqrt(a1 * a1 + b1 * b1);
    double C2 = std::sqrt(a2 * a2 + b2 * b2);
    double avg_C = (C1 + C2) / 2.0;

    double G = 0.5 * (1.0 - std::sqrt(std::pow(avg_C, 7.0) / (std::pow(avg_C, 7.0) + std::pow(25.0, 7.0))));
    double a1p = (1.0 + G) * a1;
    double a2p = (1.0 + G) * a2;

    double C1p = std::sqrt(a1p * a1p + b1 * b1);
    double C2p = std::sqrt(a2p * a2p + b2 * b2);

    auto get_hp = [](double b, double ap) {
        if (b == 0 && ap == 0) return 0.0;
        double h = std::atan2(b, ap) * 180.0 / std::numbers::pi;
        return (h >= 0) ? h : (h + 360.0);
    };

    double h1p = get_hp(b1, a1p);
    double h2p = get_hp(b2, a2p);

    double dLp = L2 - L1;
    double dCp = C2p - C1p;
    double dhp = 0.0;
    if (C1p * C2p != 0) {
        if (std::abs(h2p - h1p) <= 180.0) dhp = h2p - h1p;
        else if (h2p - h1p > 180.0) dhp = h2p - h1p - 360.0;
        else dhp = h2p - h1p + 360.0;
    }
    double dHp = 2.0 * std::sqrt(C1p * C2p) * std::sin(dhp * std::numbers::pi / 360.0);

    double avg_Lp = (L1 + L2) / 2.0;
    double avg_Cp = (C1p + C2p) / 2.0;
    double avg_hp = 0.0;
    if (C1p * C2p != 0) {
        if (std::abs(h1p - h2p) <= 180.0) avg_hp = (h1p + h2p) / 2.0;
        else if (h1p + h2p < 360.0) avg_hp = (h1p + h2p + 360.0) / 2.0;
        else avg_hp = (h1p + h2p - 360.0) / 2.0;
    }

    double T = 1.0 - 0.17 * std::cos((avg_hp - 30.0) * std::numbers::pi / 180.0) +
               0.24 * std::cos((2.0 * avg_hp) * std::numbers::pi / 180.0) +
               0.32 * std::cos((3.0 * avg_hp + 6.0) * std::numbers::pi / 180.0) -
               0.20 * std::cos((4.0 * avg_hp - 63.0) * std::numbers::pi / 180.0);

    double dtheta = 30.0 * std::exp(-std::pow((avg_hp - 275.0) / 25.0, 2.0));
    double RC = 2.0 * std::sqrt(std::pow(avg_Cp, 7.0) / (std::pow(avg_Cp, 7.0) + std::pow(25.0, 7.0)));
    double SL = 1.0 + (0.015 * std::pow(avg_Lp - 50.0, 2.0)) / std::sqrt(20.0 + std::pow(avg_Lp - 50.0, 2.0));
    double SC = 1.0 + 0.045 * avg_Cp;
    double SH = 1.0 + 0.015 * avg_Cp * T;
    double RT = -std::sin(2.0 * dtheta * std::numbers::pi / 180.0) * RC;

    return std::sqrt(std::pow(dLp / (kL * SL), 2.0) +
                     std::pow(dCp / (kC * SC), 2.0) +
                     std::pow(dHp / (kH * SH), 2.0) +
                     RT * (dCp / (kC * SC)) * (dHp / (kH * SH)));
}

std::vector<PaletteColor> parse_palette(const std::string& palette_str) {
    std::vector<PaletteColor> palette;
    std::stringstream ss(palette_str);
    std::string item;
    while (std::getline(ss, item, ';')) {
        if (item.empty()) continue;
        
        Color color = {0, 0, 0, 255, false};
        if (item.find(',') != std::string::npos) {
            std::stringstream css(item);
            std::string val;
            std::vector<int> components;
            while (std::getline(css, val, ',')) {
                try { components.push_back(std::stoi(val)); } catch (...) {}
            }
            if (components.size() >= 3) {
                color.r = static_cast<unsigned char>(components[0]);
                color.g = static_cast<unsigned char>(components[1]);
                color.b = static_cast<unsigned char>(components[2]);
                if (components.size() >= 4) {
                    color.a = static_cast<unsigned char>(components[3]);
                    color.has_alpha = true;
                }
            }
        } else {
            std::string hex_item = item;
            if (hex_item.starts_with("0x") || hex_item.starts_with("0X")) hex_item = hex_item.substr(2);
            else if (hex_item.starts_with("#")) hex_item = hex_item.substr(1);

            if (hex_item.size() >= 6) {
                try {
                    unsigned long hex_val = std::stoul(hex_item, nullptr, 16);
                    if (hex_item.size() == 6) {
                        color.r = (hex_val >> 16) & 0xFF;
                        color.g = (hex_val >> 8) & 0xFF;
                        color.b = hex_val & 0xFF;
                    } else if (hex_item.size() == 8) {
                        color.r = (hex_val >> 24) & 0xFF;
                        color.g = (hex_val >> 16) & 0xFF;
                        color.b = (hex_val >> 8) & 0xFF;
                        color.a = hex_val & 0xFF;
                        color.has_alpha = true;
                    }
                } catch (...) {}
            }
        }
        palette.push_back({color, rgb_to_lab(color.r, color.g, color.b)});
    }
    return palette;
}

Color find_closest_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a, const std::vector<PaletteColor>& palette) {
    if (palette.empty()) return {r, g, b, a, true};
    
    Lab target_lab = rgb_to_lab(r, g, b);
    double min_dist = 1e18;
    Color closest = palette[0].color;
    
    for (const auto& p : palette) {
        double dist = delta_e_2000(target_lab, p.lab);
        if (dist < min_dist) {
            min_dist = dist;
            closest = p.color;
        }
    }
    
    if (!closest.has_alpha) closest.a = a;
    return closest;
}

void output_pixels(const unsigned char* data, int width, int height, const OutputConfig& config) {
    int num_threads = std::max(1u, std::thread::hardware_concurrency());
    if (width * height < 10000) num_threads = 1;

    std::vector<std::string> results(num_threads);
    std::vector<std::future<void>> futures;

    int rows_per_thread = height / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int start_y = t * rows_per_thread;
        int end_y = (t == num_threads - 1) ? height : (t + 1) * rows_per_thread;

        futures.push_back(std::async(std::launch::async, [=, &results, &data, &config]() {
            std::stringstream ss;
            for (int y = start_y; y < end_y; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = (y * width + x) * 4;
                    unsigned char r = data[idx];
                    unsigned char g = data[idx + 1];
                    unsigned char b = data[idx + 2];
                    unsigned char a = data[idx + 3];

                    if (config.show_coords) {
                        ss << x << "," << y << ": ";
                    }

                    if (config.use_rgb) {
                        if (config.is_opaque) {
                            ss << (int)r << "," << (int)g << "," << (int)b << "\n";
                        } else {
                            ss << (int)r << "," << (int)g << "," << (int)b << "," << std::fixed << std::setprecision(1) << (a / 255.0) << "\n";
                        }
                    } else if (config.use_bgr) {
                        if (config.is_opaque) {
                            ss << (int)b << "," << (int)g << "," << (int)r << "\n";
                        } else {
                            ss << (int)b << "," << (int)g << "," << (int)r << "," << std::fixed << std::setprecision(1) << (a / 255.0) << "\n";
                        }
                    } else {
                        ss << config.prefix;
                        ss << std::uppercase << std::hex << std::setfill('0');
                        ss << std::setw(2) << (int)r << std::setw(2) << (int)g << std::setw(2) << (int)b;
                        if (!config.is_opaque) {
                            ss << std::setw(2) << (int)a;
                        }
                        ss << std::dec << "\n";
                    }
                }
            }
            results[t] = ss.str();
        }));
    }

    for (auto& f : futures) f.wait();
    for (const auto& res : results) std::cout << res;
}

std::vector<unsigned char> process_image_to_buffer(const unsigned char* data, int width, int height, const OutputConfig& config) {
    int num_threads = std::max(1u, std::thread::hardware_concurrency());
    if (width * height < 10000) num_threads = 1;

    std::vector<std::vector<unsigned char>> thread_buffers(num_threads);
    std::vector<std::future<void>> futures;

    int rows_per_thread = height / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int start_y = t * rows_per_thread;
        int end_y = (t == num_threads - 1) ? height : (t + 1) * rows_per_thread;

        futures.push_back(std::async(std::launch::async, [=, &thread_buffers, &data, &config]() {
            thread_buffers[t].reserve((end_y - start_y) * width * 4);
            for (int y = start_y; y < end_y; ++y) {
                for (int x = 0; x < width; ++x) {
                    int idx = (y * width + x) * 4;
                    unsigned char r = data[idx];
                    unsigned char g = data[idx + 1];
                    unsigned char b = data[idx + 2];
                    unsigned char a = data[idx + 3];

                    if (!config.palette.empty()) {
                        Color closest = find_closest_color(r, g, b, a, config.palette);
                        r = closest.r;
                        g = closest.g;
                        b = closest.b;
                        a = closest.a;
                    }

                    if (config.is_opaque) {
                        if (a == 0 && config.palette.empty()) {
                            r = g = b = 0;
                        }
                        a = 255;
                    }

                    thread_buffers[t].push_back(r);
                    thread_buffers[t].push_back(g);
                    thread_buffers[t].push_back(b);
                    thread_buffers[t].push_back(a);
                }
            }
        }));
    }

    for (auto& f : futures) f.wait();

    std::vector<unsigned char> full_buffer;
    full_buffer.reserve(width * height * 4);
    for (const auto& buffer : thread_buffers) {
        full_buffer.insert(full_buffer.end(), buffer.begin(), buffer.end());
    }
    return full_buffer;
}

int handle_image(const ArgumentParser& parser) {
    if (parser.get_argument_count() == 0) {
        std::cerr << "Error: No image file specified.\n";
        parser.print_help(HELP_HEADER, true);
        return 1;
    }

    std::string filename = parser.get_arguments()[0];
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (!data) {
        std::cerr << "Error: Failed to load image '" << filename << "'.\n";
        return 1;
    }

    int status = 0;
    if (parser.is_switch_set("W") || parser.is_switch_set("width")) {
        std::cout << width << "\n";
    } else if (parser.is_switch_set("H") || parser.is_switch_set("height")) {
        std::cout << height << "\n";
    } else if (parser.is_switch_set("r") || parser.is_switch_set("resolution")) {
        std::cout << width << "x" << height << "\n";
    } else {
        OutputConfig config = {
            parser.is_switch_set("rgb"),
            parser.is_switch_set("BGR"),
            parser.is_switch_set("o") || parser.is_switch_set("opaque"),
            parser.is_switch_set("c") || parser.is_switch_set("coordinates"),
            parser.get_switch_value("p").value_or(parser.get_switch_value("prefix").value_or("")),
            {},
            parser.get_switch_value("O").value_or(parser.get_switch_value("output").value_or(""))
        };

        auto palette_str = parser.get_switch_value("C").value_or(parser.get_switch_value("closest-list").value_or(""));
        if (!palette_str.empty()) {
            config.palette = parse_palette(palette_str);
        }

        // 1. Process original pixels (palette matching, etc.)
        std::vector<unsigned char> processed_buffer = process_image_to_buffer(data, width, height, config);

        unsigned char* final_data = processed_buffer.data();
        int final_w = width;
        int final_h = height;
        std::vector<unsigned char> resized_buffer;

        // 2. Resize if requested
        auto resize_str = parser.get_switch_value("R").value_or(parser.get_switch_value("resize").value_or(""));
        if (!resize_str.empty()) {
            size_t x_pos = resize_str.find('x');
            if (x_pos != std::string::npos) {
                try {
                    int target_w = std::stoi(resize_str.substr(0, x_pos));
                    int target_h = std::stoi(resize_str.substr(x_pos + 1));
                    resized_buffer.resize(target_w * target_h * 4);

                    // We use stbir_resize_uint8_srgb on the PROCESSED data
                    stbir_resize_uint8_srgb(processed_buffer.data(), width, height, 0, resized_buffer.data(), target_w, target_h, 0, STBIR_RGBA);

                    final_data = resized_buffer.data();
                    final_w = target_w;
                    final_h = target_h;
                } catch (...) {
                    std::cerr << "Error: Invalid resize format. Use <WIDTH>x<HEIGHT>.\n";
                }
            } else {
                std::cerr << "Error: Invalid resize format. Use <WIDTH>x<HEIGHT>.\n";
            }
        }

        // 3. Output
        if (!config.output_filename.empty()) {
            int success = 0;
            std::string ext = config.output_filename.substr(config.output_filename.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == "png") success = stbi_write_png(config.output_filename.c_str(), final_w, final_h, 4, final_data, final_w * 4);
            else if (ext == "bmp") success = stbi_write_bmp(config.output_filename.c_str(), final_w, final_h, 4, final_data);
            else if (ext == "tga") success = stbi_write_tga(config.output_filename.c_str(), final_w, final_h, 4, final_data);
            else if (ext == "jpg" || ext == "jpeg") success = stbi_write_jpg(config.output_filename.c_str(), final_w, final_h, 4, final_data, 90);
            else success = stbi_write_png(config.output_filename.c_str(), final_w, final_h, 4, final_data, final_w * 4);

            if (!success) {
                std::cerr << "Error: Failed to write image to '" << config.output_filename << "'.\n";
            }
        } else {
            output_pixels(final_data, final_w, final_h, config);
        }
    }

    stbi_image_free(data);
    return status;
}
int run(int argc, char* argv[]) {
    ArgumentParser parser(argc, argv);
    setup_parser(parser);

    try {
        parser.parse();
    } catch (const ArgumentParserException& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (parser.is_switch_set("v") || parser.is_switch_set("version")) {
        std::cout << "imggetpixels version " << PROJECT_VERSION << "\n";
        return 0;
    }

    if (parser.is_switch_set("h") || parser.is_switch_set("help")) {
        parser.print_help(HELP_HEADER, true);
        return 0;
    }

    return handle_image(parser);
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
