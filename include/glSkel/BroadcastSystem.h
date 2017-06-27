#pragma once

#include <vector>
#include <algorithm>

class Object;

namespace BroadcastSystem
{
	enum EVENT {
		MOUSE_CLICK,
		MOUSE_UNCLICK,
		MOUSE_MOVE,
		MOUSE_SCROLL,
		KEY_PRESS,
		KEY_UNPRESS,
		SHRINK_RAY,
		GROW_RAY
	};

	class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void receiveEvent(Object* obj, const int event, void* data) = 0;	
	};

	class Broadcaster
	{
	public:
		void attach(Listener *obs)
		{
			listeners.push_back(obs);
		}

		void detach(Listener *obs)
		{
			listeners.erase(std::remove(listeners.begin(), listeners.end(), obs), listeners.end());
		}

	protected:
		virtual void notify(Object* obj, const int event, void* data = NULL)
		{
			for (auto obs : listeners) obs->receiveEvent(obj, event, data);
		}

	private:
		std::vector<Listener *> listeners;
	};
 }