#include "TableModel.h"

#include <stdexcept>

TableModel::TableModel(const TableRow& headerTitles)
  : _headerTitles{ headerTitles }
{
}

const TableData& TableModel::headerData(size_t col) const
{
  if (col < colCount())
    return _headerTitles[col];
  else
    throw std::out_of_range("Invalid column number.");
}

bool TableModel::setData(size_t row, size_t col, const TableData& data)
{
  if ((row < rowCount()) && (col < colCount()))
  {
    (*std::next(_rows.begin(), row))[col] = data;
    return true;
  }
  return false;
}

const TableData& TableModel::data(size_t row, size_t col) const
{
  if ((row < rowCount()) && (col < colCount()))
    return (*std::next(_rows.begin(), row))[col];
  else
    throw std::out_of_range("Invalid row or column number.");
}
