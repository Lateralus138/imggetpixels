#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include "pch.h"
#include "stb_image.h"

#include <future>
#include <sstream>
#include "version.h"

using namespace argparser;

/*
void print_help(const ArgumentParser& parser) {
    (void)parser;
    std::cout << "USAGE: imggetpixels [SWITCH|ARG <PARAM>] IMAGE_FILE\n"
              << "Get colors and size information of an image file in\n"
              << "various formats. Values default to bare hexadecimal\n"
              << "format with transparency (e.g. FFFFFFFF).\n\n"
              << "Args/flags:\n"
              << "  -v,--version      Show the version of the utility.\n"
              << "  -h,--help         This help message.\n"
              << "  --rgb             Output is in RGB with transparency.\n"
              << "                    E.g. 255,127,0,0.1.\n"
              << "  --BGR             Output is in BGR with transparency.\n"
              << "                    E.g. 255,127,0,0.1.\n"
              << "  -o,--opaque       Output has no transparency values.\n"
              << "                    E.g. 7F3FAF.\n"
              << "  -p,--prefix       Prepend a string to the output of\n"
              << "                    hexadecimal values. E.g. -p \"0x\"\n"
              << "                    would output 0x7F3FAF.\n"
              << "  -c,--coordinates  Return the output in the format of\n"
              << "                    x,y: <COLOR>. E.g. 902,35: FFFFFF.\n"
              << "  -r,--resolution   Only return the width and height\n"
              << "                    of the image in <WIDTH>x<HEIGHT>\n"
              << "                    format.\n"
              << "  -W,--width        Only return the image's width.\n"
              << "  -H,--height       Only return the image's height.\n";
}
*/

const char* HELP_HEADER = "Get colors and size information of an image file in\n"
                          "various formats. Values default to bare hexadecimal\n"
                          "format with transparency (e.g. FFFFFFFF).";

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
}

struct OutputConfig {
    bool use_rgb;
    bool use_bgr;
    bool is_opaque;
    bool show_coords;
    std::string prefix;
};

void process_image_pixels(unsigned char* data, int width, int height, const OutputConfig& config) {
    int num_threads = std::max(1u, std::thread::hardware_concurrency());
    // Heuristic: only use multiple threads for images larger than a certain size
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

                    if (config.is_opaque && a == 0) {
                        r = g = b = 0;
                    }

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

    for (auto& f : futures) {
        f.wait();
    }

    for (const auto& res : results) {
        std::cout << res;
    }
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    try {
        ArgumentParser parser(argc, argv);
        setup_parser(parser);
        parser.parse();

        if (parser.is_switch_set("v") || parser.is_switch_set("version")) {
            std::cout << "imggetpixels version " << PROJECT_VERSION << "\n";
            return 0;
        }

        if (parser.is_switch_set("h") || parser.is_switch_set("help")) {
            parser.print_help(HELP_HEADER, true);
            return 0;
        }

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
                parser.get_switch_value("p").value_or(parser.get_switch_value("prefix").value_or(""))
            };
            process_image_pixels(data, width, height, config);
        }

        stbi_image_free(data);
    } catch (const ArgumentParserException& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
