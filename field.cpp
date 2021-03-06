#include "field.h"
#include "window.h"     // For debugmsg
#include "stringfunc.h" // For no_caps and trim
#include "terrain.h"    // For Terrain_flag
#include "globals.h"    // For TERRAIN and FIELDS
#include "rng.h"
#include "map.h"
#include "entity.h"
#include "game.h"
#include <sstream>

Field_fuel::Field_fuel(Terrain_flag TF, Item_flag IF, int _fuel, Dice _damage)
{
  terrain_flag = TF;
  item_flag = IF;
  fuel = _fuel;
  damage = _damage;
  any_item = false;
  has_explosion = false;
}

bool Field_fuel::load_data(std::istream& data, std::string owner_name)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {
    if ( ! (data >> ident) ) {
      debugmsg("Couldn't read ident for fuel (%s)", owner_name.c_str());
      return false;
    }
    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') {
// It's a comment
      std::getline(data, junk);

    } else if (ident == "terrain_flag:") {
      std::string flagname;
      std::getline(data, flagname);
      flagname = trim(flagname);
      Terrain_flag tf = lookup_terrain_flag(flagname);
      if (tf == TF_NULL) {
        debugmsg("Unknown terrain flag '%s' (%s)", flagname.c_str(),
                 owner_name.c_str());
        return false;
      }
      terrain_flag = tf;
//debugmsg("Field_fueld loaded terrain_flag '%s'", flagname.c_str());

    } else if (ident == "item_flag:") {
      std::string flagname;
      std::getline(data, flagname);
      flagname = trim(flagname);
      Item_flag itf = lookup_item_flag(flagname);
      if (itf == ITEM_FLAG_NULL) {
        debugmsg("Unknown item flag '%s' (%s)", flagname.c_str(),
                 owner_name.c_str());
        return false;
      }
      item_flag = itf;
//debugmsg("Field_fueld loaded item_flag '%s'", flagname.c_str());

    } else if (ident == "any_item") {
      any_item = true;
      std::getline(data, junk);
//debugmsg("Field_fuel loaded any_item");

    } else if (ident == "fuel:") {
      data >> fuel;
      std::getline(data, junk);
//debugmsg("Field_fuel loaded fuel %d", fuel);

    } else if (ident == "damage:") {
      if (!damage.load_data(data, owner_name + " fuel")) {
        return false;
      }
//debugmsg("Field_fuel loaded damage %s", damage.str().c_str());

    } else if (ident == "output_field:") {
      std::getline(data, output_field);
      output_field = trim(output_field);
//debugmsg("Field_fuel loaded output_fiel %s", output_field.c_str());

    } else if (ident == "output_duration:") {
      if (!output_duration.load_data(data, owner_name + " fuel")) {
        return false;
      }
//debugmsg("Field_fuel loaded output_duration %s", output_duration.str().c_str());

    } else if (ident == "explosion:") {
      if (!explosion.load_data(data, owner_name + " fuel")) {
        debugmsg("Explosion failed to load.");
        return false;
      }
      has_explosion = true;
//debugmsg("Field_fuel loaded explosion");

    } else if (ident != "done") {
      debugmsg("Unknown Field_terrain_fuel property '%s' (%s)", ident.c_str(),
               owner_name.c_str());
      return false;
    }
  }
//debugmsg("Field_fuel successfully loaded.");
  return true;
}
  
Field_level::Field_level()
{
  duration = 0;
  duration_lost_without_fuel = 0;
  for (int i = 0; i < TF_MAX; i++) {
    terrain_flags.push_back(false);
  }
  for (int i = 0; i < FIELD_FLAG_MAX; i++) {
    field_flags.push_back(false);
  }
}

Field_level::~Field_level()
{
}

bool Field_level::load_data(std::istream& data, std::string owner_name)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {
    if ( ! (data >> ident) ) {
      if (!name.empty()) {
        debugmsg("Field data file terminated unexpectedly while loading level \
for '%s'.", owner_name.c_str());
      }
      return false;
    }
    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') {
// It's a comment
      std::getline(data, junk);

    } else if (ident == "name:") {
      if (!name.empty()) {
        std::string new_name;
        std::getline(data, new_name);
        new_name = trim(new_name);
        debugmsg("Field_level '%s' loading name again? (%s)", name.c_str(),
                 new_name.c_str());
      }
      std::getline(data, name);
      name = trim(name);
//debugmsg("Field_level loaded name '%s'", name.c_str());

    } else if (ident == "glyph:") {
      sym.load_data_text(data);
      std::getline(data, junk);
//debugmsg("Field_level loaded glyph %s", sym.text_formatted().c_str());

    } else if (ident == "duration:") {
      data >> duration;
      std::getline(data, junk);
//debugmsg("Field_level loaded duration %d", duration);

    } else if (ident == "duration_lost_without_fuel:") {
      data >> duration_lost_without_fuel;
      std::getline(data, junk);
//debugmsg("Field_level loaded duration_lost_without_fuel %d", duration_lost_without_fuel);

    } else if (ident == "fuel:") {
      Field_fuel tmpfuel;
      if (tmpfuel.load_data(data, name)) {
        fuel.push_back(tmpfuel);
      } else {
        debugmsg("Fuel failed to load (%s)", owner_name.c_str());
        return false;
      }
//debugmsg("Field_level loaded fuel successfully.");

    } else if (ident == "danger:") {
      data >> danger;
      std::getline(data, junk);
//debugmsg("Field_level loaded danger %d", danger);

    } else if (ident == "verb:") {
      std::getline(data, verb);
      verb = trim(verb);
//debugmsg("Field_level loaded verb '%s'", verb.c_str());

    } else if (ident == "parts_hit:") {
      std::string body_part_line;
      std::getline(data, body_part_line);
      std::istringstream body_part_data(body_part_line);
      std::string body_part_name;
      while (body_part_data >> body_part_name) {
        std::vector<Body_part> parts = get_body_part_list( body_part_name );
        for (int i = 0; i < parts.size(); i++) {
          body_parts_hit.push_back( parts[i] );
        }
        if (parts.empty()) {
          debugmsg("Unknown body part '%s' (%s:%s)", body_part_name.c_str(),
                   owner_name.c_str(), name.c_str());
          return false;
        }
      }
//debugmsg("Field_level loaded parts_hit '%s'", body_part_line.c_str());

    } else if (ident == "flags:") {
/* TODO:  Should terrain flags and field flags be loaded with different property
 *        tags?  Normally it's not an issue, but if there's a name collision we
 *        obviously need to be able to distinguish between the two.
 */
      std::string flag_line;
      std::getline(data, flag_line);
      std::istringstream flag_data(flag_line);
      std::string flag_name;
      while (flag_data >> flag_name) {
        Terrain_flag tf = lookup_terrain_flag(flag_name);
        if (tf == TF_NULL) {
          Field_flag ff = lookup_field_flag(flag_name);
          if (ff == FIELD_FLAG_NULL) {
            debugmsg("'%s' is not a terrain nor a field flag (%s:%s)",
                     flag_name.c_str(), owner_name.c_str(), name.c_str());
            return false;
          } else {
            field_flags[ff] = true;
          }
        } else {
          terrain_flags[tf] = true;
        }
      }
//debugmsg("Field_level loaded flags '%s'", flag_line.c_str());

    } else if (ident != "done") {
/* Check if it's a damage type; TODO: make this less ugly (duplicated in
 *                                    attack.cpp!)
 * Damage_set::load_data() maybe?
 */
      std::string damage_name = ident;
      size_t colon = damage_name.find(':');
      if (colon != std::string::npos) {
        damage_name = damage_name.substr(0, colon);
      }
      Damage_type type = lookup_damage_type(damage_name);
      if (type == DAMAGE_NULL) {
        debugmsg("Unknown Field_level property '%s' (%s:%s)", ident.c_str(),
                 owner_name.c_str(), name.c_str());
        return false;
      } else {
        int tmpdam;
        data >> tmpdam;
        damage.set_damage(type, tmpdam);
//debugmsg("Field_level loaded damage '%s' %d", damage_name.c_str(), tmpdam);
      }
    }

  }
//debugmsg("Field_level '%s' loaded successfully.", name.c_str());
  return true;
}

std::string Field_level::get_name()
{
  return name;
}

bool Field_level::has_flag(Field_flag ff)
{
  return field_flags[ff];
}

bool Field_level::has_flag(Terrain_flag tf)
{
  return terrain_flags[tf];
}

Field_type::Field_type()
{
  uid = -1;
  spread_chance = 0;
  for (int i = 0; i < TF_MAX; i++) {
    terrain_flags.push_back(false);
  }
  for (int i = 0; i < FIELD_FLAG_MAX; i++) {
    field_flags.push_back(false);
  }
}

Field_type::~Field_type()
{
  for (int i = 0; i < levels.size(); i++) {
    delete (levels[i]);
  }
}

std::string Field_type::get_data_name()
{
  return name;
}

std::string Field_type::get_name()
{
  if (display_name.empty()) {
    return name;
  }
  return display_name;
}

std::string Field_type::get_level_name(int level)
{
  if (level < 0 || level >= levels.size()) {
    std::stringstream ret;
    ret << "ERROR: Out-of-bounds " << get_name() << " [" << level << "]";
    return ret.str();
  }

  return levels[level]->get_name();
}

Field_level* Field_type::get_level(int level)
{
  if (level < 0 || level >= levels.size()) {
    return NULL;
  }
  return levels[level];
}

bool Field_type::has_flag(Terrain_flag tf, int level)
{
  if (terrain_flags[tf]) {
    return true;
  }
  if (level >= 0 && level < levels.size() && get_level(level)->has_flag(tf)) {
    return true;
  }
  return false;
}

bool Field_type::has_flag(Field_flag ff, int level)
{
  if (field_flags[ff]) {
    return true;
  }
  if (level >= 0 && level < levels.size() && get_level(level)->has_flag(ff)) {
    return true;
  }
  return false;
}

int Field_type::duration_needed_to_reach_level(int level)
{
  if (level <= 0 || level >= levels.size()) {
    return 999999;
  }
  return levels[level - 1]->duration + levels[level]->duration;
}

int Field_type::get_uid()
{
  return uid;
}

bool Field_type::load_data(std::istream& data)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {
    if ( ! (data >> ident) ) {
      if (!name.empty()) {
        debugmsg("Field data file terminated unexpectedly while loading '%s'.",
                 name.c_str());
      }
      return false;
    }
    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') {
// It's a comment
      std::getline(data, junk);

    } else if (ident == "name:") {
      if (!name.empty()) {
        std::string new_name;
        std::getline(data, new_name);
        new_name = trim(new_name);
        debugmsg("Field_type '%s' loading name again? (%s)", name.c_str(),
                 new_name.c_str());
      }
      std::getline(data, name);
      name = trim(name);
//debugmsg("Field_type loaded name '%s'", name.c_str());

    } else if (ident == "display_name:") {
      std::getline(data, display_name);
      display_name = trim(display_name);
//debugmsg("Field_type loaded display_name '%s'", display_name.c_str());

    } else if (ident == "flags:") {
/* TODO:  Should terrain flags and field flags be loaded with different property
 *        tags?  Normally it's not an issue, but if there's a name collision we
 *        obviously need to be able to distinguish between the two.
 */
      std::string flag_line;
      std::getline(data, flag_line);
      std::istringstream flag_data(flag_line);
      std::string flag_name;
      while (flag_data >> flag_name) {
        Terrain_flag tf = lookup_terrain_flag(flag_name);
        if (tf == TF_NULL) {
          Field_flag ff = lookup_field_flag(flag_name);
          if (ff == FIELD_FLAG_NULL) {
            debugmsg("'%s' is not a terrain nor a field flag (%s)",
                     flag_name.c_str(), name.c_str());
            return false;
          } else {
            field_flags[ff] = true;
          }
        } else {
          terrain_flags[tf] = true;
        }
      }
//debugmsg("Field_type loaded flags '%s'", flag_line.c_str());

    } else if (ident == "spread_chance:") {
      data >> spread_chance;
      if (spread_chance < 0 || spread_chance > 100) {
        debugmsg("Bad spread_chance %d (%s)", spread_chance, name.c_str());
        return false;
      }
      std::getline(data, junk);
//debugmsg("Field_type loaded spread_chance %d", spread_chance);

    } else if (ident == "spread_cost:") {
      data >> spread_cost;
      std::getline(data, junk);
//debugmsg("Field_type loaded spread_cost %d", spread_cost);

    } else if (ident == "output_chance:") {
      data >> output_chance;
      if (output_chance < 0 || output_chance > 100) {
        debugmsg("Bad output_chance %d (%s)", output_chance, name.c_str());
        return false;
      }
      std::getline(data, junk);
//debugmsg("Field_type loaded output_chance %d", output_chance);

    } else if (ident == "output_cost:") {
      data >> output_cost;
      if (output_cost < 0 || output_cost > 100) {
        debugmsg("Bad output_cost %d (%s)", output_cost, name.c_str());
        return false;
      }
      std::getline(data, junk);
//debugmsg("Field_type loaded output_cost %d", output_cost);

    } else if (ident == "output_type:") {
      std::getline(data, output_type);
      output_type = trim(output_type);
//debugmsg("Field_type loaded output_type '%s'", output_type.c_str());

    } else if (ident == "fuel:") {
      Field_fuel tmpfuel;
      if (tmpfuel.load_data(data, name)) {
        fuel.push_back(tmpfuel);
      } else {
        debugmsg("Fuel failed to load (%s)", name.c_str());
        return false;
      }
//debugmsg("Field_type successfully loaded fuel.");

    } else if (ident == "level:") {
      Field_level* tmp_level = new Field_level;
      std::stringstream name_to_pass;
      name_to_pass << name << " level " << levels.size();
      if (!tmp_level->load_data(data, name_to_pass.str())) {
        delete tmp_level;
        debugmsg("Field level failed to load (%s)", name.c_str());
        return false;
      }
      levels.push_back(tmp_level);
//debugmsg("Field_type successfully loaded level.");

    } else if (ident != "done") {
      debugmsg("Unknown Field_type property: '%s' (%s)", ident.c_str(),
               name.c_str());
      return false;
    }
  }
//debugmsg("Field_type '%s' loaded successfully.", name.c_str());
  return true;
}

Field_pool::Field_pool()
{
  type = NULL;
  duration = Dice(0, 0, 0);
  tiles = Dice(0, 0, 1);
}

Field_pool::~Field_pool()
{
}

bool Field_pool::exists()
{
  return type;
}

void Field_pool::drop(Tripoint pos, std::string creator)
{
  int num_tiles = tiles.roll();
  if (num_tiles <= 0) {
    return; // Don't drop anything!
  }
  num_tiles--; // The one at the exact point is the first
  int dur = duration.roll();
  Field tmp(type);
  tmp.set_duration(dur);
  if (!creator.empty()) {
    tmp.creator = creator;
  }
  GAME.map->add_field(tmp, pos);

  for (int i = 0; i < num_tiles; i++) {
    Tripoint next_pos(pos.x + rng(-1, 1), pos.y + rng(-1, 1), pos.z);
    dur = duration.roll();
    tmp.set_duration(dur);
    GAME.map->add_field(tmp, next_pos);
  }
}

bool Field_pool::load_data(std::istream& data, std::string owner_name)
{
  std::string ident, junk;
  while (ident != "done" && !data.eof()) {

    if ( ! (data >> ident) ) {
      return false;
    }

    ident = no_caps(ident);

    if (!ident.empty() && ident[0] == '#') { // I'ts a comment
      std::getline(data, junk);

    } else if (ident == "type:") {
      std::string field_name;
      std::getline(data, field_name);
      field_name = trim(field_name);
      type = FIELDS.lookup_name(field_name);
      if (!type) {
        debugmsg("Unknown field '%s' in Field_pool (%s)", field_name.c_str(),
                 owner_name.c_str());
        return false;
      }

    } else if (ident == "duration:") {
      duration.load_data(data, owner_name + " Field_pool duration");

    } else if (ident == "tiles:") {
      tiles.load_data(data, owner_name + " Field_pool tiles");

    } else if (ident != "done") {
      debugmsg("Unknown Field_pool property '%s' (%s)", ident.c_str(),
               owner_name.c_str());
      return false;
    }
  }
  return true;
}

Field::Field(Field_type* T, int L, std::string C)
{
  type = T;
  level = L;
  creator = C;
  duration = 0;
  dead = false;
  if (type) {
    Field_level* lev = type->get_level(level);
    if (lev) {
      duration = lev->duration;
    }
  }
}

Field::~Field()
{
}

int Field::get_type_uid() const
{
  if (!type) {
    return -1;
  }
  return type->uid;
}

bool Field::is_valid()
{
  if (!type) {
    return false;
  }
  if (level < 0) {
    return false;
  }
  if (dead) {
    return false;
  }
  return true;
}

bool Field::has_flag(Field_flag flag)
{
  if (!type) {
    return false;
  }
  return type->has_flag(flag, level);
}

bool Field::has_flag(Terrain_flag flag)
{
  if (!type) {
    return false;
  }
  return type->has_flag(flag, level);
}

std::string Field::get_name()
{
  if (!type) {
    return "BUG - Typeless field";
  }
  Field_level* lev = type->get_level(level);
  if (!lev) {
    return "BUG - Invalid field level";
  }
  return lev->name;
}

std::string Field::get_full_name()
{
  std::stringstream ret;
  ret << get_name();
  if (!creator.empty()) {
    ret << " (created by " << creator << ")";
  }
  return ret.str();
}

glyph Field::top_glyph()
{
  if (!type) {
    return glyph();
  }
  Field_level* lev = type->get_level(level);
  return lev->sym;
}

int Field::get_full_duration() const
{
  int ret = duration;
  if (!type) {
    return ret;
  }
  for (int i = 0; i < level; i++) {
    ret += type->get_level(i)->duration;
  }
  return ret;
}

Field& Field::operator+=(const Field& rhs)
{
  if (get_type_uid() != rhs.get_type_uid()) {
    return (*this);
  }
  duration += rhs.get_full_duration();
  adjust_level();
  return (*this);
}

void Field::set_duration(int dur)
{
  duration = dur;
  adjust_level();
}

void Field::hit_entity(Entity* entity)
{
  if (!entity || !type) {
    return;
  }
  Field_level* lev = type->get_level(level);
  if (!lev) {
    return;
  }

  std::list<Body_part>* bp_hit = &(lev->body_parts_hit);
  if (bp_hit->empty()) {
    return;
  }
  for (std::list<Body_part>::iterator it = bp_hit->begin();
       it != bp_hit->end();
       it++) {
    entity->take_damage(lev->damage, get_full_name(), (*it));
  }
// TODO: Status effects etc.
}

/* Field::process handles its per-turn effects:
 *  Lose 1 duration
 *  Consume fuel, if any is available
 *  Lose extra duration, if our type calls for it, if no fuel was consumed
 *  Change level, if appropriate
 */
void Field::process(Map* map, Tripoint pos)
{
  if (!map) {
    debugmsg("Field::process() called with map == NULL!");
    return;
  }
  if (!type) {
    debugmsg("Field::process() called for a Field without any type!");
    return;
  }

// Fetch info on the map and our type data
  Field_level* level_data = type->get_level(level);

  duration--;

  bool found_fuel = consume_fuel(map, pos);
// Lose extra duration if we didn't find fuel
  if (!found_fuel && level_data) {
    duration -= level_data->duration_lost_without_fuel;
  }

// Lose duration if we rise and there's empty terrain above
// TODO: Don't hard-code duration lost?
// TODO: Actual vertical spreading?
  if (has_flag(FIELD_FLAG_RISE) &&
      map->has_flag(TF_OPEN_SPACE, pos.x, pos.y, pos.z + 1)) {
    duration -= rng(0, 30);
  }
// Spread if appropriate

// Calculate which points are open for spreading
  if ((has_flag(FIELD_FLAG_DIFFUSE) ||
       get_full_duration() > type->spread_cost) &&
      rng(1, 100) <= type->spread_chance) {
    bool solid = has_flag(FIELD_FLAG_SOLID);
    std::vector<Tripoint> spread_points;
    for (int x = pos.x - 1; x <= pos.x + 1; x++) {
      for (int y = pos.y - 1; y <= pos.y + 1; y++) {
        int z = pos.z;
        if ((!map->contains_field(x, y, z) ||
            map->field_uid_at(x, y, z) == get_type_uid()) &&
            (solid || map->move_cost(x, y, z) > 0))  {
          spread_points.push_back( Tripoint(x, y, z) );
        }
      }
    }

    if (!spread_points.empty()) {
      Tripoint spread = spread_points[ rng(0, spread_points.size() - 1) ];
      map->add_field(type, spread, creator);
      duration -= type->spread_cost;
    }
  }

// Change our level, if appropriate!
  adjust_level();

// TODO: Output extra stuff (output_type from type)
}

void Field::gain_level()
{
  if (!type) {
    return;
  }
  Field_level* lev = type->get_level(level);
  if (!lev) {
    return;
  }
  duration -= lev->duration;
  level++;
}

void Field::lose_level()
{
  if (level == 0) {
    dead = true;
  }
  level--;
  if (!type) {
    return;
  }
  Field_level* lev = type->get_level(level);
  if (!lev) {
    return;
  }
  duration = lev->duration;
}

void Field::adjust_level()
{
  if (!type) {
    return;
  }

  while (duration < 0 && level >= 0) {
    level--;
    if (level < 0) {
      dead = true;
    } else {
      duration += type->get_level(level)->duration;
    }
  }

  while (duration >= type->duration_needed_to_reach_level(level + 1)) {
    duration -= type->get_level(level)->duration;
    level++;
  }

  if (duration < 0) {
    dead = true;
  }
}

bool Field::consume_fuel(Map* map, Tripoint pos)
{
// Calculate which points are open - in case we spread or put out smoke/etc
  std::vector<Tripoint> open_points_all;
  std::vector<Tripoint> open_points_passable; // i.e. not walls
  for (int x = pos.x - 1; x <= pos.x + 1; x++) {
    for (int y = pos.y - 1; y <= pos.y + 1; y++) {
      int z = pos.z;
      if (!map->contains_field(x, y, z)) {
        open_points_all.push_back( Tripoint(x, y, z) );
        if (map->move_cost(x, y, z) > 0) {
          open_points_passable.push_back( Tripoint(x, y, z) );
        }
      }
    }
  }

  bool found_fuel = false;  // Our return value
  std::vector<Field>  output; // Fields we output (e.g. smoke)

// Check for fuel/extinguishers of the TYPE
  for (std::list<Field_fuel>::iterator it = type->fuel.begin();
       it != type->fuel.end();
       it++) {
    Field_fuel fuel = (*it);
    found_fuel = (found_fuel || look_for_fuel(fuel, map, pos, output));
  }
// Check for fuel/extinguishers of the LEVEL
  Field_level* lev = type->get_level(level);
  if (lev) {
    for (std::list<Field_fuel>::iterator it = lev->fuel.begin();
         it != lev->fuel.end();
         it++) {
      Field_fuel fuel = (*it);
      found_fuel = (found_fuel || look_for_fuel(fuel, map, pos, output));
    }
  }

// At this point we've consumed fuel; so output whatever "smoke" we have
  for (int i = 0; i < output.size(); i++) {
    std::vector<Tripoint>* placement;
// Decide whether to place on all valid points, or passable ones
    if ( output[i].has_flag(FIELD_FLAG_SOLID) ) {
      placement = &(open_points_all);
    } else {
      placement = &(open_points_passable);
    }
    if (placement->empty()) {
      i = output.size();
    } else {
      int index = rng(0, placement->size() - 1);
      Tripoint p = (*placement)[index];
      placement->erase( placement->begin() + index );
      map->add_field(output[i], p);
    }
  }

  return found_fuel;
}

bool Field::look_for_fuel(Field_fuel fuel, Map* map, Tripoint pos,
                          std::vector<Field>& output)
{
  bool found_fuel = false;  // Our return value
// Get info on the tile and our field data
  Tile* tile_here = map->get_tile(pos);

// Create a new field that we're going to output
  Field_type* smoke_type = FIELDS.lookup_name(fuel.output_field);
  if (!fuel.output_field.empty() && !smoke_type) {
    debugmsg("Couldn't find field type '%s' (Field::consume_fuel() for '%s'",
             fuel.output_field.c_str(), get_name().c_str());
    return false;
  }
  Field tmp_field(smoke_type, 1, creator);
  tmp_field.duration = 0;

// Check for terrain flag
  if (tile_here->has_flag(fuel.terrain_flag)) {
    int fuel_gained = fuel.fuel;
    int damage = fuel.damage.roll();
    if (fuel_gained >= 0) {
      found_fuel = true;
    }
    if (tile_here->hp < damage) {
      double fuel_percent = tile_here->hp / damage;
      fuel_gained = int( double(fuel.fuel) * fuel_percent );
    }
    duration += fuel_gained;
    tmp_field.duration += fuel.output_duration.roll();
// TODO: Assign a damage type to Field_terrain_fuel?
    tile_here->damage(DAMAGE_NULL, damage);
  }
// Check items at this tile
  for (int n = 0; n < tile_here->items.size(); n++) {
    Item* it = &(tile_here->items[n]);
    if (it->has_flag(fuel.item_flag)) {
      int fuel_gained = fuel.fuel;
      int damage = fuel.damage.roll();
      if (fuel_gained >= 0) {
        fuel_gained = true;
      }
      if (it->hp < damage) {
        double fuel_percent = it->hp / damage;
        fuel_gained = int( double(fuel.fuel) * fuel_percent );
      }
      duration += fuel_gained;
      tmp_field.duration += fuel.output_duration.roll();
      if (it->damage(damage)) { // Item was destroyed
        tile_here->items.erase( tile_here->items.begin() + n );
        n--;
      }
    }
  }
// Push our output to the vector of new fields
// Ensure tmp_field.duration > 0 because sometimes it rolls < 0
  if (tmp_field.type && tmp_field.duration > 0) {
    tmp_field.adjust_level();
    output.push_back(tmp_field);
  }
  return found_fuel;
}

Field_flag lookup_field_flag(std::string name)
{
  name = no_caps(name);
  name = trim(name);
  for (int i = 0; i < FIELD_FLAG_MAX; i++) {
    Field_flag ret = Field_flag(i);
    if ( no_caps( field_flag_name(ret) ) == name ) {
      return ret;
    }
  }
  return FIELD_FLAG_NULL;
}

std::string field_flag_name(Field_flag flag)
{
  switch (flag) {
    case FIELD_FLAG_NULL:     return "NULL";
    case FIELD_FLAG_SOLID:    return "solid";
    case FIELD_FLAG_DIFFUSE:  return "diffuse";
    case FIELD_FLAG_RISE:     return "rise";
    case FIELD_FLAG_MAX:      return "BUG - FIELD_FLAG_MAX";
    default:                  return "BUG - Unnamed Field_flag";
  }
  return "BUG - Escaped field_flag_name switch";
}
