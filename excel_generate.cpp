#include "main.h"
#include "excel_generate.h"
#include <xlnt/xlnt.hpp>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <cstdio> // Для функции remove()
#include "database.h"

std::string generate_excel_report(const std::string& trade_point) {
    // 1. Получаем данные из БД
    std::vector<ApplicationDataForReport> data = db_get_apps_data_for_report(trade_point);

    // 2. Создаем новую рабочую книгу Excel
    xlnt::workbook wb;
    xlnt::worksheet ws = wb.active_sheet();
    ws.title("Заявки");

    // 3. Заполняем заголовки
    ws.cell("A1").value("ID Заявки");
    ws.cell("B1").value("Дата и время");
    ws.cell("C1").value("Имя клиента");
    ws.cell("D1").value("Телефон");
    ws.cell("E1").value("Почта");
    ws.cell("F1").value("Тариф");
    ws.cell("G1").value("Стоимость");
    ws.cell("H1").value("Адрес");
    ws.cell("I1").value("ID клиента в TG");

    // 4. Заполняем данными
    int row = 2;
    for (const auto& app : data) {
        ws.cell(1, row).value(app.id);
        ws.cell(2, row).value(app.timestamp);
        ws.cell(3, row).value(app.name);
        ws.cell(4, row).value(app.phone);
        ws.cell(5, row).value(app.email);
        ws.cell(6, row).value(app.tariff);
        ws.cell(7, row).value(app.price);
        ws.cell(8, row).value(app.address);
        ws.cell(9, row).value(app.user_id);
        row++;
    }

    // 5. Генерируем уникальное имя файла и сохраняем
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "report_" << trade_point << "_" << in_time_t << ".xlsx";
    std::string filename = ss.str();

    wb.save(filename);

    // 6. Возвращаем имя созданного файла
    return filename;
}