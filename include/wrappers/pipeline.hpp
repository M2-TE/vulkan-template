#pragma once

struct Pipeline {
	Pipeline(std::string pathCompute);
	Pipeline(std::string pathVertex, std::string pathPixel);
	void init();

	inline void write_desc() {

	}

private:
	struct Shader {
		Shader(std::string);
		void init();
	};
	std::vector<Shader> shaders;
};