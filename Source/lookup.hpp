#ifndef lookup_h
#define lookup_h

#include <vector>
#include <utility>
#include <algorithm>

namespace cobalt {
	template <typename Key, typename Value>
	class lookup {
	public:
		using valueType = std::pair<Key, Value>;
		using containerType = std::vector<valueType>;
	private:
		containerType _container;
	public:
		using iterator = typename containerType::const_iterator;
		using const_iterator = iterator;
		
		lookup(std::initializer_list<valueType> init) :
			_container(init)
		{
			std::sort(_container.begin(), _container.end());
		}
		
		lookup(containerType container) :
			_container(std::move(container))
		{
			std::sort(_container.begin(), _container.end());
		}
		
		const_iterator begin() const {
			return _container.begin();
		}
		
		const_iterator end() const {
			return _container.end();
		}
		
		template <typename K>
		const_iterator find(const K& key) const {
			const_iterator it = std::lower_bound(
				begin(),
				end(),
				key,
				[](const valueType& p, const K& key) {
					return p.first < key;
				}
			);
			return it != end() && it->first == key ? it : end();
		}
		
		size_t size() const {
			return _container.size();
		}
	};
}

#endif /* lookup_h */
