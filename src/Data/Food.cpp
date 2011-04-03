#include "Data/Food.h"

#include "Data/Corpse.h"

using namespace data;
using namespace std;

/* initialize static variables */
map<const string, const Food*> Food::_foods;

/* protected constructors */
Food::Food(const string& name, int cost, int weight, int material, unsigned long long properties, int nutrition, int time, int effects) : Item(name, cost, weight, FOOD, material, properties), _nutrition(nutrition), _time(time), _effects(effects) {
}

/* destructor */
Food::~Food() {
}

/* public static methods */
void Food::init() {
	create("meatball", 5, 1, MATERIAL_FLESH, 0, 5, 1, 0);
	create("meat ring", 5, 1, MATERIAL_FLESH, 0, 5, 1, 0);
	create("meat stick", 5, 1, MATERIAL_FLESH, 0, 5, 1, 0);
	create("egg", 9, 15, MATERIAL_FLESH, 0, 80, 1, EAT_EFFECT_VEGAN);
	create("tripe ration", 15, 10, MATERIAL_FLESH, 0, 200, 2, EAT_EFFECT_STUN | EAT_EFFECT_CONFUSE);
	create("huge chunk of meat", 105, 400, MATERIAL_FLESH, 0, 2000, 20, 0);
	create("kelp frond", 6, 1, MATERIAL_VEGGY, 0, 30, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("eucalyptus leaf", 6, 1, MATERIAL_VEGGY, 0, 30, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN | EAT_EFFECT_CURE_SICKNESS);
	create("clove of garlic", 7, 1, MATERIAL_VEGGY, 0, 40, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("sprig of wolfsbane", 7, 1, MATERIAL_VEGGY, 0, 40, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN | EAT_EFFECT_CURE_LYCANTHROPY);
	create("apple", 7, 2, MATERIAL_VEGGY, 0, 50, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("carrot", 7, 2, MATERIAL_VEGGY, 0, 50, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN | EAT_EFFECT_CURE_BLINDNESS);
	create("pear", 7, 2, MATERIAL_VEGGY, 0, 50, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("banana", 9, 2, MATERIAL_VEGGY, 0, 80, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("orange", 9, 2, MATERIAL_VEGGY, 0, 80, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("melon", 10, 5, MATERIAL_VEGGY, 0, 100, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("byte", 17, 5, MATERIAL_VEGGY, 0, 250, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("fortune cookie", 7, 1, MATERIAL_VEGGY, 0, 40, 1, EAT_EFFECT_VEGETARIAN);
	create("candy bar", 10, 2, MATERIAL_VEGGY, 0, 100, 1, EAT_EFFECT_VEGETARIAN);
	create("cream pie", 10, 10, MATERIAL_VEGGY, 0, 100, 1, EAT_EFFECT_VEGETARIAN);
	create("lump of royal jelly", 15, 2, MATERIAL_VEGGY, 0, 200, 1, EAT_EFFECT_VEGETARIAN | EAT_EFFECT_STRENGTH);
	create("pancake", 15, 2, MATERIAL_VEGGY, 0, 200, 2, EAT_EFFECT_VEGETARIAN);
	create("C-ration", 20, 10, MATERIAL_VEGGY, 0, 300, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("K-ration", 25, 10, MATERIAL_VEGGY, 0, 400, 1, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("cram ration", 35, 15, MATERIAL_VEGGY, 0, 600, 3, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("food ration", 45, 20, MATERIAL_VEGGY, 0, 800, 5, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("gunyoki", 45, 20, MATERIAL_VEGGY, 0, 800, 5, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("lembas wafer", 45, 5, MATERIAL_VEGGY, 0, 800, 2, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN);
	create("tin", 5, 10, MATERIAL_VEGGY, 0, 50, 50, 0);
	create("tin of spinach", 5, 10, MATERIAL_VEGGY, 0, 50, 50, EAT_EFFECT_VEGAN | EAT_EFFECT_VEGETARIAN | EAT_EFFECT_STRENGTH);

	Corpse::init();
}

const map<const string, const Food*>& Food::foods() {
	return _foods;
}

/* public methods */
int Food::nutrition() const {
	return _nutrition;
}

int Food::time() const {
	return _time;
}

int Food::effects() const {
	return _effects;
}

/* protected static methods */
void Food::addToMap(const string& name, const Food* food) {
	_foods[name] = food;
	Item::addToMap(name, food);
}

/* private static methods */
void Food::create(const string& name, int cost, int weight, int material, unsigned long long properties, int nutrition, int time, int effects) {
	addToMap(name, new Food(name, cost, weight, material, properties, nutrition, time, effects | EAT_EFFECT_NEVER_ROT));
	if (time > 1 && name != "tin" && name != "tin of spinach") {
		string partly_eaten = "partly eaten ";
		partly_eaten.append(name);
		addToMap(partly_eaten, new Food(partly_eaten, 0, weight / 2, material, properties, nutrition / 2, time / 2, effects | EAT_EFFECT_NEVER_ROT));
	}
}

