#ifndef DATA_ITEM_H
#define DATA_ITEM_H

#include "Data/Material.h"
#include <set>
#include <map>
#include <string>

namespace data {
	class Item;
	typedef std::set<const data::Item*> ItemSet;

	class Item {
	public:
		virtual ~Item();

		static void init();
		static void destroy();
		static const std::map<const std::string, const Item*>& items();
		const std::string& name() const;
		int cost() const;
		int weight() const;
		char category() const;
		int material() const;
		unsigned long long properties() const;

	protected:
		Item(const std::string& name, int cost, int weight, char category, int material, unsigned long long properties);

		static void addToMap(const std::string& name, const Item* item);

	private:
		static std::map<const std::string, const Item*> _items;
		const std::string _name;
		const int _cost;
		const int _weight;
		const char _category;
		const int _material;
		const unsigned long long _properties;
	};
}
#endif
