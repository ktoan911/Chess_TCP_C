// common/include/json_handler.hpp
#ifndef JSON_HANDLER_HPP
#define JSON_HANDLER_HPP

#include <string>
#include <fstream>
#include <iostream>

#include "../libraries/json.hpp"

// Lớp để xử lý JSON
class JSONHandler
{
public:
    static nlohmann::json readJSON(const std::string &filename)
    {
        try
        {
            std::ifstream infile(filename);
            if (!infile.is_open())
            {
                std::cerr << "Không thể mở file " << filename << " để đọc JSON." << std::endl;
                return nlohmann::json::object();
            }

            if (infile.peek() == std::ifstream::traits_type::eof())
            {
                nlohmann::json emptyJson = nlohmann::json::object();
                infile >> emptyJson;

                infile.close();
                return emptyJson;
            }

            nlohmann::json j;
            infile >> j;

            return j;
        }
        catch (const nlohmann::json::parse_error &e)
        {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return nlohmann::json::object();
        }
    }

    static void writeJSON(const std::string &filename, const nlohmann::json &j)
    {
        try
        {
            std::ofstream outfile(filename);

            if (!outfile.is_open())
            {
                std::cerr << "Không thể mở file " << filename << " để ghi JSON." << std::endl;
                return;
            }

            outfile << j.dump(4);
            outfile.close();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Lỗi khi ghi JSON: " << e.what() << std::endl;
        }
    }
};

#endif // JSON_HANDLER_HPP