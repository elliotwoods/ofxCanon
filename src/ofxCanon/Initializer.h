#pragma once

namespace ofxCanon {
	class Initializer {
	public:
		static Initializer & X();

		Initializer();
		~Initializer();

		bool isInitialized() const;
	protected:
		bool initialized = false;
	};
}