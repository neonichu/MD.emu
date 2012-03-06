#pragma once

template <class C>
class RefCount
{
	uint count;

public:
	constexpr RefCount(): count(0) { }

	void ref()
	{
		count++;
	}

	void freeRef()
	{
		assert(count);
		count--;
		if(!count)
		{
			static_cast<C*>(this)->free();
		}
	}
};
