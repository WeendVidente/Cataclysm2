#include "world_terrain.h"
#include "stringfunc.h"
#include "window.h"

World_terrain::World_terrain()
{
  uid = -1;
  name = "ERROR";
  sym = glyph();
}

bool World_terrain::load_data(std::istream &data)
{
  std::string ident, junk;
  do {
    if ( ! (data >> ident) ) {
      return false;
    }

    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') {
// It's a comment
      std::getline(data, junk);

    } else if (ident == "name:") {
      std::getline(data, name);
      name = trim(name);

    } else if (ident == "glyph:") {
      sym.load_data_text(data);
      std::getline(data, junk);

    } else if (ident != "done") {
      debugmsg("Unknown World_terrain property '%s' (%s)",
               ident.c_str(), name.c_str());
    }
  } while (ident != "done" && !data.eof());
// TODO: Flag loading.
  return true;
}
