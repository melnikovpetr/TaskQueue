#ifndef TABLE_MODEL_H
#define TABLE_MODEL_H

#include <list>
#include <memory>
#include <string>
#include <vector>

using TableData = std::string;
using TableRow = std::vector<TableData>;

class TableModel
{
public:
  TableModel(const TableRow& headerTitles);

  auto colCount() const noexcept { return _headerTitles.size(); };
  auto rowCount() const noexcept { return _rows.size(); };
  auto begin() const noexcept { return _rows.begin(); }
  auto end() const noexcept { return _rows.end(); }

  void appendRow() { _rows.push_back(TableRow(colCount())); }
  void removeLastRow() { _rows.pop_back(); }

  const TableData& headerData(size_t col) const;
  bool setData(size_t row, size_t col, const TableData& data);
  const TableData& data(size_t row, size_t col) const;

private:
  TableRow _headerTitles;
  std::list<TableRow> _rows;
};

#endif // TABLE_MODEL_H
