#pragma once

namespace ofxCanon {
	class Initializer {
	public:
		static Initializer & X();

		Initializer();
		~Initializer();
		
		void init();

		bool isInitialized() const;
	protected:
		bool initialized = false;
	};
}
