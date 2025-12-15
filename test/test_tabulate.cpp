// main.cpp
#include "tabulate.hpp"
#include <iostream>

int main()
{
    using namespace tabulate;

    // 1. Creating a Basic Table
    Table basic_table;

    // 2. Adding Headers and Rows
    basic_table.add_row({"IDDDDDD", "Name", "Ageeeeeeee", "Occupation"});
    basic_table.add_row({"1", "Alice", "30", "Engineer"});
    basic_table.add_row({"2", "Bob", "25", "Designer"});
    basic_table.add_row({"3", "Charlie", "35", "Teacher"});

    // 3. Styling the Table
    basic_table.format()
        .font_style({FontStyle::bold})
        .font_align(FontAlign::center)
        .border_color(Color::blue)
        .corner_color(Color::blue)
        .padding_left(2)
        .padding_right(2)
        .border_top("═")
        .border_bottom("═")
        .border_left("║")
        .border_right("║")
        .corner("╬");

    std::cout << "=== Basic Table with Styling ===" << std::endl;
    std::cout << basic_table << std::endl;

    // 4. Cell Formatting
    Table cell_formatted_table;
    cell_formatted_table.add_row({"Product", "Price", "Stock"});
    cell_formatted_table.add_row({"Laptop", "$999", "Available"});
    cell_formatted_table.add_row({"Smartphone", "$499", "Out of Stock"});
    cell_formatted_table.add_row({"Tablet", "$299", "Limited"});

    // Formatting individual cells
    cell_formatted_table[1][2].format().font_color(Color::red).font_style({FontStyle::bold});
    cell_formatted_table[2][2].format().font_color(Color::yellow).font_style({FontStyle::italic});
    cell_formatted_table[3][2].format().font_color(Color::green).font_style({FontStyle::underline});

    cell_formatted_table.format()
        .font_align(FontAlign::left)
        // .border_top("═")
        // .border_bottom("═")
        // .border_left("║")
        // .border_right("║")
        // .corner("╬")
        .padding_left(1)
        .padding_right(1);

    std::cout << "=== Cell Formatted Table ===" << std::endl;
    std::cout << cell_formatted_table << std::endl;

    // 6. Nested Tables
    Table nested_table;
    nested_table.add_row({"Employee", "Details"});

    // Creating a nested table for Alice
    Table alice_details;
    alice_details.add_row({"Age", "30"});
    alice_details.add_row({"Occupation", "Engineer"});
    alice_details.add_row({"Location", "New York"});

    // Creating a nested table for Bob
    Table bob_details;
    bob_details.add_row({"Age", "25"});
    bob_details.add_row({"Occupation", "Designer"});
    bob_details.add_row({"Location", "San Francisco"});

    // Adding nested tables as cells
    nested_table.add_row({"Alice", alice_details});
    nested_table.add_row({"Bob", bob_details});

    nested_table.format()
        .font_align(FontAlign::left)
        .border_color(Color::cyan)
        .padding_left(1)
        .padding_right(1);

    std::cout << "=== Nested Tables ===" << std::endl;
    std::cout << nested_table << std::endl;

    // UTF-8 Example Table
    Table utf8_table;

    utf8_table.format()
        .corner("+")
        .font_style({FontStyle::bold})
        .corner_color(Color::magenta)
        .border_color(Color::magenta);

    utf8_table.add_row({"English", "I love you"});
    utf8_table.add_row({"French", "Je t’aime"});
    utf8_table.add_row({"Spanish", "Te amo"});
    utf8_table.add_row({"German", "Ich liebe Dich"});
    utf8_table.add_row({"Mandarin Chinese", "我爱你"});
    utf8_table.add_row({"Japanese", "愛してる"});
    utf8_table.add_row({"Korean", "사랑해 (Saranghae)"});
    utf8_table.add_row({"Greek", "Σ΄αγαπώ (Se agapo)"});
    utf8_table.add_row({"Italian", "Ti amo"});
    utf8_table.add_row({"Russian", "Я тебя люблю (Ya tebya liubliu)"});
    utf8_table.add_row({"Hebrew", "אני אוהב אותך (Ani ohev otakh)"});

    // Column 1 is using mult-byte characters
    utf8_table.column(1).format().multi_byte_characters(true);

    std::cout << "=== UTF-8 Supported Table ===" << std::endl;
    std::cout << utf8_table << std::endl;

    Table range_based_table;

    range_based_table.add_row({"Company", "Contact", "Country"});
    range_based_table.add_row({"Alfreds Futterkiste", "Maria Anders", "Germany"});
    range_based_table.add_row({"Centro comercial Moctezuma", "Francisco Chang", "Mexico"});
    range_based_table.add_row({"Ernst Handel", "Roland Mendel", "Austria"});
    range_based_table.add_row({"Island Trading", "Helen Bennett", "UK"});
    range_based_table.add_row({"Laughing Bacchus Winecellars", "Yoshi Tannamuri", "Canada"});
    range_based_table.add_row({"Magazzini Alimentari Riuniti", "Giovanni Rovelli", "Italy"});

    // Set width of cells in each column
    range_based_table.column(0).format().width(40);
    range_based_table.column(1).format().width(30);
    range_based_table.column(2).format().width(30);

    // Iterate over cells in the first row
    for (auto &cell : range_based_table[0])
    {
        cell.format()
            .font_style({FontStyle::underline})
            .font_align(FontAlign::center);
    }

    // Iterator over cells in the first column
    for (auto &cell : range_based_table.column(0))
    {
        if (cell.get_text() != "Company")
        {
            cell.format()
                .font_align(FontAlign::right);
        }
    }

    // Iterate over rows in the table
    size_t index = 0;
    for (auto &row : range_based_table)
    {
        row.format()
            .font_style({FontStyle::bold});

        // Set blue background color for alternate rows
        if (index > 0 && index % 2 == 0)
        {
            for (auto &cell : row)
            {
                cell.format()
                    .font_background_color(Color::blue);
            }
        }
        index += 1;
    }

    std::cout << "=== Range-based formatted Table ===" << std::endl;
    std::cout << range_based_table << std::endl;

    return 0;
}
