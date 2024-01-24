#include "../include/ui_navigation.h"
#include "../include/ui.h"

const Navigation::Page &Navigation::getPage() const { return page; }
void Navigation::setPage(Navigation::Page location) {
  page = location;
  ui->refreshLayout();
}
