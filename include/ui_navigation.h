#pragma once

#include "collider.h"
#include "widget.h"
class UserInterface;
class Navigation {
public:
  enum Page { INSTRUMENT, SETTINGS, NEW_GAME };
  Navigation(UserInterface *_ui) : ui(_ui) {}
  const Page &getPage() const;
  void setPage(Navigation::Page location);

private:
  Page page = INSTRUMENT;
  UserInterface *ui = NULL;
  // AbstractUI *activeModal = NULL;
};
static const int NUM_NAVIGATION_PAGES = 3;
static_assert((NUM_NAVIGATION_PAGES - 1) == Navigation::NEW_GAME,
              "Navigation enum size does not match NavigationPages");
constexpr static const Navigation::Page NavigationPages[NUM_NAVIGATION_PAGES] =
    {Navigation::INSTRUMENT, Navigation::SETTINGS, Navigation::NEW_GAME};

constexpr static const char *NavigationPageDisplayNames[NUM_NAVIGATION_PAGES] =
    {"play instrument", "settings", "new game"};
