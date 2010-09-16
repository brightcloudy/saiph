#include "Data/Wand.h"
#include "Globals.h"

#define WAND_VANISHER_MESSAGE " vanishes!  " //The engraving on the <floor/ground/etc> vanishes!
#define WAND_COLD_MESSAGE "  A few ice cubes drop from the wand.  "
#define WAND_SLEEP_DEATH_MESSAGE "  The bugs on the floor stop moving!  "
#define WAND_MAGIC_MISSILE_MESSAGE "  The floor is riddled by bullet holes!  "
#define WAND_POLYMORPH_MESSAGE "  The engraving now reads: " //followed by random rumor
#define WAND_SLOW_MONSTER_MESSAGE "  The bugs on the floor slow down!  "
#define WAND_SPEED_MONSTER_MESSAGE "  The bugs on the floor speed up!  "
#define WAND_STRIKING_MESSAGE "  The wand unsuccessfully fights your attempt to write!  "
#define WAND_DIGGING_MESSAGE " is a wand of digging!  "
#define WAND_LIGHTNING_MESSAGE " is a wand of lightning!  "
#define WAND_FIRE_MESSAGE " is a wand of fire!  "
#define WAND_CREATE_MONSTER_MESSAGE "  You write in the dust with a wand of create monster.  "
#define WAND_ENLIGHTENMENT_MESSAGE "  You feel self-knowledgeable...  "
#define WAND_LIGHT_MESSAGE "  A lit field surrounds you!  "
#define WAND_WISHING_MESSAGE "  You may wish for an object.  "

using namespace data;

std::map<const std::string, const Wand*> Wand::_wands;
std::vector<std::string> Wand::_wand_appearances;

Wand::Wand(const std::string& name, int cost, int material, int maximum_charges, int zap_type, const std::string& engrave_message, bool autoidentifies_on_engrave, unsigned long long properties)
		: Item(name, cost, 7, WAND, material, properties), _maximum_charges(maximum_charges), _zap_type(zap_type), _engrave_message(engrave_message), _autoidentifies_on_engrave(autoidentifies_on_engrave) {
}

Wand::~Wand() {
}

void Wand::init() {
	create("wand of light", 100, MATERIAL_UNKNOWN, 18, WAND_ZAP_TYPE_NONDIRECTIONAL, WAND_LIGHT_MESSAGE, true, PROPERTY_MAGIC);
	create("wand of nothing", 100, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, "", false, 0);

	create("wand of digging", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_DIGGING_MESSAGE, true, PROPERTY_MAGIC);
	create("wand of enlightenment", 150, MATERIAL_UNKNOWN, 15, WAND_ZAP_TYPE_NONDIRECTIONAL, "", true, PROPERTY_MAGIC);
	create("wand of locking", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, "", false, PROPERTY_MAGIC);
	create("wand of magic missile", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_MAGIC_MISSILE_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of make invisible", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_VANISHER_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of opening", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, "", false, PROPERTY_MAGIC);
	create("wand of probing", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, "", false, PROPERTY_MAGIC);
	create("wand of secret door detection", 150, MATERIAL_UNKNOWN, 15, WAND_ZAP_TYPE_NONDIRECTIONAL, "", false, PROPERTY_MAGIC);
	create("wand of slow monster", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_SLOW_MONSTER_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of speed monster", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_SPEED_MONSTER_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of striking", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_STRIKING_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of undead turning", 150, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, "", false, PROPERTY_MAGIC);

	create("wand of cold", 175, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_COLD_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of fire", 175, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_FIRE_MESSAGE, true, PROPERTY_MAGIC);
	create("wand of lightning", 175, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_LIGHTNING_MESSAGE, true, PROPERTY_MAGIC);
	create("wand of sleep", 175, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_SLEEP_DEATH_MESSAGE, false, PROPERTY_MAGIC);

	create("wand of cancellation", 200, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_VANISHER_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of create monster", 200, MATERIAL_UNKNOWN, 15, WAND_ZAP_TYPE_NONDIRECTIONAL, "", true, PROPERTY_MAGIC);
	create("wand of polymorph", 200, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_POLYMORPH_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of teleportation", 200, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_BEAM, WAND_VANISHER_MESSAGE, false, PROPERTY_MAGIC);

	create("wand of death", 500, MATERIAL_UNKNOWN, 8, WAND_ZAP_TYPE_RAY, WAND_SLEEP_DEATH_MESSAGE, false, PROPERTY_MAGIC);
	create("wand of wishing", 500, MATERIAL_UNKNOWN, 3, WAND_ZAP_TYPE_NONDIRECTIONAL, "", true, PROPERTY_MAGIC);

	_wand_appearances.push_back("marble wand");
	_wand_appearances.push_back("glass wand");
	_wand_appearances.push_back("crystal wand");
	_wand_appearances.push_back("balsa wand");
	_wand_appearances.push_back("maple wand");
	_wand_appearances.push_back("oak wand");
	_wand_appearances.push_back("pine wand");
	_wand_appearances.push_back("ebony wand");
	_wand_appearances.push_back("runed wand");
	_wand_appearances.push_back("copper wand");
	_wand_appearances.push_back("silver wand");
	_wand_appearances.push_back("platinum wand");
	_wand_appearances.push_back("iron wand");
	_wand_appearances.push_back("long wand");
	_wand_appearances.push_back("short wand");
	_wand_appearances.push_back("curved wand");
	_wand_appearances.push_back("forked wand");
	_wand_appearances.push_back("hexagonal wand");
	_wand_appearances.push_back("spiked wand");
	_wand_appearances.push_back("jeweled wand");
	_wand_appearances.push_back("tin wand");
	_wand_appearances.push_back("brass wand");
	_wand_appearances.push_back("iridium wand");
	_wand_appearances.push_back("zinc wand");
	_wand_appearances.push_back("aluminum wand");
	_wand_appearances.push_back("uranium wand");
	_wand_appearances.push_back("steel wand");
}

const std::map<const std::string, const Wand*>& Wand::items() {
	return _wands;
}

const std::vector<std::string>& Wand::appearances() {
	return _wand_appearances;
}

int Wand::maximumCharges() const {
	return _maximum_charges;
}

int Wand::zapType() const {
	return _zap_type;
}

const std::string& Wand::engraveMessage() const {
	return _engrave_message;
}

bool Wand::autoidentifiesOnEngraving() const {
	return _autoidentifies_on_engrave;
}

void Wand::addToMap(const std::string& name, const Wand* wand) {
	_wands[name] = wand;
	Item::addToMap(name, wand);
}

void Wand::create(const std::string& name, int cost, int material, int maximum_charges, int zap_type, const std::string& engrave_message, bool autoidentifies_on_engrave, unsigned long long properties) {
	addToMap(name, new Wand(name, cost, material, maximum_charges, zap_type, engrave_message, autoidentifies_on_engrave, properties));
}
