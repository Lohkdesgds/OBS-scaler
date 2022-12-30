#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <functional>

const std::string app_version = "V1.1";
const std::string obs_path = "\\obs-studio\\basic\\scenes";

const std::string json_tags[] = {"width", "height", "x", "y" };
const std::string json_ptags[] = {"pos", "scale", "bounds", "settings" };
const std::string json_tag_contains = "resolution"; // format: NNNNxNNNN
const std::string json_ignore_all[] = {"active_session_list", "id_counter", "device_id", "color_range", "color_space", "capture", "decode", "reroute_audio", "url", "file", "ratio", "time", "threshold", "frame", "interval", "fps"};
const std::string json_ignore_all_f[] = {"id", "db"};
const char ignore_on_check_matchs[][2] = {
	{' ', '_'}
};

std::string getenv_safe(const std::string& env);
std::string unwrap_backslash(std::string wrk);
std::string file_name_of(const std::string& pth);
std::string read_file(const std::string& path);
void find_scenes_on(const nlohmann::json& j, std::vector<std::string>& v);
void find_browsers_on(const nlohmann::json& j, std::vector<std::string>& v);
void recursive_json_do(nlohmann::json& j, const std::function<void(const std::string&, const std::string&, nlohmann::json&, const uint16_t)>& f, const std::function<uint16_t(const nlohmann::json&)>& avoid_f = [](auto){ return 0; }, const std::string& key = {}, const std::string& pkey = {}, uint16_t flaggd = 0);
void clear_cin();
void wait();
void logstr(const std::string&);
bool is_equal_ignore_auto(const std::string&, const std::string&);

int main()
{
	const std::string appdata_path = getenv_safe("APPDATA");
	if (appdata_path.empty()) {
		std::cout << "Could not get APPDATA path. It is empty?" << std::endl;
		wait();
		return 1;
	}

	logstr("Version: " + app_version);
	logstr("Any issues please talk to Lohk on:");
	logstr("Twitter: @lohkdesgds");
	logstr("Started app.");
	logstr("Environment: " + appdata_path);

	const std::string obs_work = appdata_path + obs_path;// unwrap_backslash(appdata_path + obs_path);
	std::vector<std::string> files;

	std::cout << "Build: " << app_version << std::endl;
	std::cout << "Using OBS path: '" << obs_work << "'" << std::endl;
	std::error_code ec;

	logstr("Work path: " + obs_work);

	for (const auto& i : std::filesystem::recursive_directory_iterator{ obs_work, ec })
	{
		if (const auto spth = i.path().string(); i.is_regular_file() && spth.find(".json") == spth.length() - sizeof(".json") + 1) {
			//std::cout << "- " << spth << std::endl;
			files.push_back(spth);
		}
	}

	if (ec) {
		logstr("Got error: " + std::to_string(ec.value()) + " - " + ec.message());
		std::cout << "Got error searching for files: #" << ec.value() << "\nDescription of the error: " << ec.message() << std::endl;
		wait();
		return 1;
	}
	if (files.size() == 0) {
		logstr("Got error: no files");
		std::cout << "Found no files in the folder." << std::endl;
		wait();
		return 1;
	}

	std::cout << "Found " << files.size() << " scene(s). Select one to manage by typing its number:" << std::endl;

	logstr("Found " + std::to_string(files.size()) + " file(s) successfully.");

	for (size_t p = 0; p < files.size(); ++p) {
		std::cout << "#" << p << ": " << file_name_of(files[p]) << std::endl;
	}

	// get width, height, pos.x, pos.y

	size_t index{}; // good
	double scaling = 1.0; // good
	nlohmann::json fj; // good
	std::string newnam; // good
	std::fstream preopfp; // good
	std::vector<std::string> scene_names;
	std::vector<std::string> browser_names;

	do {
		std::cout << "> ";
		clear_cin();
		std::cin >> index;

		if (index >= files.size()) {
			std::cout << "Invalid number. Please try between 0 and " << (files.size() - 1) << std::endl;
		}
	} while (index >= files.size());

	logstr("Index selected: " + std::to_string(index));

	const auto& file_indexed = files[index];
	const std::string file_name_src = file_name_of(file_indexed);

	std::cout << "Opening file '" << file_name_src << "'... ";
	logstr("Source file name: " + file_name_src);

	{
		const auto data = read_file(file_indexed);
		if (data.empty()) {
			logstr("Error: Source has no data.");
			std::cout << "Fatal error: no data" << std::endl;
			wait();
			return 1;
		}

		fj = nlohmann::json::parse(data, nullptr, false);

		if (fj.is_discarded() || fj.is_null()) {
			logstr("Error: cannot parse json.");
			std::cout << "Fatal error: data is null" << std::endl;
			wait();
			return 1;
		}
	}

	logstr("Parsed json successfully.");

	std::cout << "Good!" << std::endl;
	std::cout << "What name do you want for the new scene collection (save as, do not include .json)?" << std::endl;

	while(1) {
		std::cout << "> ";
		clear_cin();
		std::getline(std::cin, newnam);

		std::string newnam_format = newnam;
		for (auto& i : newnam_format) if (i == ' ') i = '_';

		preopfp.open(obs_work + "\\" + newnam_format + ".json", std::ios::out | std::ios::binary);
		if (!preopfp || preopfp.bad()) {
			std::cout << "Bad name, couldn't open file. Please try another name maybe." << std::endl;
		}
		else {
			logstr("Target file name: " + newnam_format);
			logstr("Target name: " + newnam);
			break;
		}
	}

	logstr("Target file name: " + newnam);
	
	{
		bool got_good = false;
		recursive_json_do(fj, [&](const std::string& pkey, const std::string& key, nlohmann::json& jj, const uint16_t in_flagged) {
			if (!got_good && key == "name" && jj.is_string() && is_equal_ignore_auto(jj.get<std::string>(), file_name_src)) {
				jj = newnam;
				got_good = true;
			}
		});

		if (!got_good) {
			std::cout << "Malformed OBS json file. Could not change scene collection name to " << newnam << " because the JSON key was not found." << std::endl;
			wait();
			return 1;
		}

		find_scenes_on(fj, scene_names);
		if (scene_names.empty()) {
			std::cout << "Note: there are no scenes on this file. Exit the app if it sounds wrong." << std::endl;
			logstr("There were no scenes in the file.");
		}

		find_browsers_on(fj, browser_names);
		if (browser_names.empty()) {
			std::cout << "Note: there are no browsers on this file. Exit the app if it sounds wrong." << std::endl;
			logstr("There were no browsers in the file.");
		}
	}

	logstr("Prepared target JSON with target file name in it");
	
	while (1) {
		int targ, orig;
		std::cout << "TARGET height?\n> ";
		clear_cin();
		std::cin >> targ;
		std::cout << "SOURCE height?\n> ";
		clear_cin();
		std::cin >> orig;

		if (targ <= 0 || orig <= 0) {
			std::cout << "One of the values does not work. Please try again." << std::endl;
		}
		else {
			logstr("Selected origin pixel height: " + std::to_string(orig));
			logstr("Selected target pixel height: " + std::to_string(targ));

			scaling = static_cast<double>(targ) * 1.0 / static_cast<double>(orig);
			std::cout << "The scaling that will be used is " << (scaling * 100.0) << "%" << std::endl;
			break;
		}
	}

	std::cout << "\nPress any key to continue with these settings:\n"
		<< "-> Source: " << file_name_of(file_indexed) << "\n"
		<< "-> Target: " << newnam << "\n"
		<< "-> Scale: " << (scaling * 100.0) << "%" << std::endl;
	
	logstr("Scale set to: " + std::to_string(scaling));

	wait();

	std::cout << "\nDoing it..." << std::endl;	
	logstr("Started task.");

	{
		size_t last_len = 0;
		std::string outt;

		recursive_json_do(fj, 
			[&](const std::string& pkey, const std::string& key, nlohmann::json& jj, const uint16_t in_flagged) {
				if (
					std::find(std::begin(json_ptags), std::end(json_ptags), pkey) == std::end(json_ptags) &&
					std::find(std::begin(json_tags), std::end(json_tags), key) == std::end(json_tags) && 
					key.find(json_tag_contains) == std::string::npos
				)
					return;
		
				if (std::find_if(std::begin(json_ignore_all), std::end(json_ignore_all),
					[&](const std::string& t) { return pkey.find(t) != std::string::npos || key.find(t) != std::string::npos; }
				) != std::end(json_ignore_all))
				{
					logstr("Skipped [FIND] \"" + pkey + "/" + key + "\": " + jj.dump());
					return;
				}
				if (std::find_if(std::begin(json_ignore_all_f), std::end(json_ignore_all_f),
					[&](const std::string& t) { return pkey == t || key == t; }
				) != std::end(json_ignore_all_f))
				{
					logstr("Skipped [FIND] \"" + pkey + "/" + key + "\": " + jj.dump());
					return;
				}

				// flag 0b10
				if (in_flagged & 0b10) {
					logstr("Skipped [FLAG 2] \"" + pkey + "/" + key + "\": " + jj.dump());
					return;
				}
				// flag 0b1
				if (pkey == "scale" && in_flagged & 0b1) {

					return;
					logstr("Skipped [FLAG 1] \"" + pkey + "/" + key + "\": " + jj.dump());
				}

				outt = "Working on: " + pkey + "/" + key + " -> " + jj.dump();
				size_t new_last_len = outt.size();
				if (outt.size() < last_len) outt.append(last_len - outt.size(), ' ');

				std::cout << outt << "\r";
				last_len = new_last_len;

				switch (jj.type()) {
				case nlohmann::json::value_t::string:
				{
					const std::string jstr = jj.get<std::string>().c_str();
					if (jstr.find('x') != std::string::npos) {

						int nums[2]{};
						if (sscanf_s(jstr.c_str(), "%dx%d", &nums[0], &nums[1]) != 2) {
							logstr("Possible issue at \"" + pkey + "/" + key + "\": " + jj.dump());
						}
						else {
							jj = std::to_string(static_cast<int64_t>(static_cast<double>(nums[0]) * scaling)) + "x" + std::to_string(static_cast<int64_t>(static_cast<double>(nums[1]) * scaling));
						}
					}
				}
					break;
				case nlohmann::json::value_t::number_integer:
				{
					const auto val = jj.get<int64_t>();
					const auto fval = static_cast<decltype(val)>(static_cast<double>(val) * scaling);
					logstr("- Changed \"" + pkey + "/" + key + "\": " + std::to_string(val) + " -> " + std::to_string(fval));
					jj = fval;
				}
				break;
				case nlohmann::json::value_t::number_unsigned:
				{
					const auto val = jj.get<uint64_t>();
					const auto fval = static_cast<decltype(val)>(static_cast<double>(val) * scaling);
					logstr("- Changed \"" + pkey + "/" + key + "\": " + std::to_string(val) + " -> " + std::to_string(fval));
					jj = fval;
				}
				break;
				case nlohmann::json::value_t::number_float:
				{
					const auto val = jj.get<float>();
					const auto fval = static_cast<decltype(val)>(static_cast<double>(val) * scaling);
					logstr("- Changed \"" + pkey + "/" + key + "\": " + std::to_string(val) + " -> " + std::to_string(fval));
					jj = fval;
				}
				break;
				default:
				{
					//outt = "[WARN] Possible bad type on \"" + pkey + "/" + key + "\": " + jj.dump() + "!";
					//if (outt.size() < last_len) outt.append(last_len - outt.size(), ' ');
					//std::cout << outt << std::endl;
					//last_len = 0;
					logstr("Possible bad type on \"" + pkey + "/" + key + "\": " + jj.dump());
				}
					break;
				}
			},
			[&scene_names, &browser_names](const nlohmann::json& j) {
				// scenes should not be scaled/transformed
				//return false;

				uint16_t flags = 0;

				if ((j.is_object() &&
					j.contains("scale") && j.contains("rot") && j.contains("visible") && j.contains("name") &&
					std::find(scene_names.begin(), scene_names.end(), j["name"]) != scene_names.end())
				) flags |= 0b1;

				if ((j.is_object() &&
					j.contains("settings") && j.contains("muted") && j.contains("volume") && j.contains("sync") &&
					std::find(browser_names.begin(), browser_names.end(), j["name"]) != browser_names.end())
				) flags |= 0b10;

				return flags;
			}
		);

		outt = "Ended working on JSON! Saving file...";
		if (outt.size() < last_len) outt.append(last_len - outt.size(), ' ');
		std::cout << outt << std::endl;
		logstr("JSON iterator ended. Saving final file...");
	}

	{
		const std::string dmp = fj.dump(1);
		preopfp.write(dmp.data(), dmp.size());
		preopfp.close();
		logstr("Saved final file.");
	}

	std::cout << "The end!" << std::endl;
	wait();
	logstr("Exited the app the wait way");

	return 0;
}

std::string getenv_safe(const std::string& env)
{
	size_t siz{};
	getenv_s(&siz, nullptr, 0, env.c_str());
	if (!siz) return {};
	std::string buf(siz, '\0');
	getenv_s(&siz, buf.data(), siz, env.c_str());
	while (buf.size() && buf.back() == '\0') buf.pop_back();
	return buf;
}

std::string unwrap_backslash(std::string wrk)
{
	for (auto& i : wrk) if (i == '\\') i = '/';
	return wrk;
}

std::string file_name_of(const std::string& pth)
{
	const size_t start = pth.rfind('\\'), endd = pth.rfind(".json");
	if (start == std::string::npos || endd == std::string::npos || endd <= start + 1) return "<unknown>";
	return pth.substr(start + 1, endd - start - 1);
}

std::string read_file(const std::string& path)
{
	std::fstream fp(path.c_str(), std::ios::in | std::ios::binary);
	if (!fp || fp.bad()) return {};
	std::string out;

	while (!fp.eof() && fp)
	{
		char blk[4096];
		fp.read(blk, std::size(blk));
		const auto rdd = fp.gcount();

		out.append(blk, rdd);
	}

	return out;
}

void find_scenes_on(const nlohmann::json& j, std::vector<std::string>& v)
{
	if (j.is_object() && j.contains("id") && j.contains("name") && j["id"] == "scene") {
		v.push_back(j["name"].get<std::string>());
	}
	else {
		if (j.is_object()) {
			for (const auto& i : j) find_scenes_on(i, v);
		}
		else if (j.is_array()) {
			for (const auto& i : j) find_scenes_on(i, v);
		}
		else if (j.is_structured()) {
			for (const auto& i : j) find_scenes_on(i, v);
		}
	}
}

void find_browsers_on(const nlohmann::json& j, std::vector<std::string>& v)
{
	if (j.is_object() && j.contains("id") && j.contains("name") && j["id"] == "browser_source") {
		v.push_back(j["name"].get<std::string>());
	}
	else {
		if (j.is_object()) {
			for (const auto& i : j) find_browsers_on(i, v);
		}
		else if (j.is_array()) {
			for (const auto& i : j) find_browsers_on(i, v);
		}
		else if (j.is_structured()) {
			for (const auto& i : j) find_browsers_on(i, v);
		}
	}
}

void recursive_json_do(nlohmann::json& j, const std::function<void(const std::string&, const std::string&, nlohmann::json&, const uint16_t)>& f, const std::function<uint16_t(const nlohmann::json&)>& avoid_f, const std::string& key, const std::string& pkey, uint16_t flaggd)
{
	if (!f || !avoid_f) return;
	flaggd |= avoid_f(j);

	if (j.is_object()) {
		for (auto it = j.begin(); it != j.end(); ++it) recursive_json_do(it.value(), f, avoid_f, it.key(), key, flaggd);
	}
	else if (j.is_array()) {
		for (auto it = j.begin(); it != j.end(); ++it) recursive_json_do(it.value(), f, avoid_f, key, pkey, flaggd);
	}
	else if (j.is_structured()) {
		for (auto it = j.begin(); it != j.end(); ++it) recursive_json_do(it.value(), f, avoid_f, it.key(), key, flaggd);
	}
	else {
		f(pkey, key, j, flaggd);
	}
	//std::cout << "\r" << key << "    ";
}

void clear_cin()
{
	while (std::cin.rdbuf()->in_avail()) 
		getchar();
}

void wait()
{
	clear_cin();
	std::cin.ignore();
}

void logstr(const std::string& str)
{
	if (str.empty()) return;
	static std::fstream of("applog.log", std::ios::out | std::ios::binary);
	if (of && of.good() && of.is_open()) of << str << "\n";
}

bool is_equal_ignore_auto(const std::string& a, const std::string& b)
{
	if (a.size() != b.size()) return false;

	for (size_t p = 0; p < a.size(); ++p) {
		const char& ac = a[p];
		const char& bc = b[p];

		if (ac == bc) continue;

		for (const auto& ii : ignore_on_check_matchs) {
			if (
				std::find(std::begin(ii), std::end(ii), ac) == std::end(ii) ||
				std::find(std::begin(ii), std::end(ii), bc) == std::end(ii)
			) 
				return false;
		}
	}
	return true;
}