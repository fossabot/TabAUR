// Functions for TabAUR, These include printing stuff and others.
// main.cpp simply pieces each function together to make the program work.
#include <git.hpp>

TaurBackend::TaurBackend() {
	git2_inits = 0;

	int status = git_clone_options_init(&git_opt, GIT_CLONE_OPTIONS_VERSION);
	if (status != 0)
		return;

	git2_inits++;
	git_libgit2_init();
}

// Wrapper for git_libgit2_shutdown, This will call the function
// multiple times until all resources are freed.
TaurBackend::~TaurBackend() {
	while (git2_inits-- > 0)
		git_libgit2_shutdown();
}

// Clone from a remote git repository, and return a git_repository object back.
// status will be set to whatever git_clone returns, 0 = success.
git_repository *TaurBackend::taur_clone_git(std::string url, std::string out_path, int *status) {
	git_repository *out = nullptr;
	*status = git_clone(&out, url.c_str(), out_path.c_str(), &git_opt);
	
	// If an error were to happen, it would just return NULL.
	return out;
}

// https://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c#4654718
bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::string getURL(rapidjson::Document& doc) {
	int resultcount = doc["resultcount"].GetInt();
	if (resultcount == 1 && doc["results"][0]["URL"].IsString()) {
		return std::string(doc["results"][0]["URL"].GetString());
	} else if (resultcount == 1) {
		return ("https://aur.archlinux.org" + std::string(doc["results"][0]["URLPath"].GetString()));
	} else if (resultcount > 1) {
		for (int i = 0; i < resultcount; i++)
			std::cout << "[" << i << "]: " << 
				doc["results"][i]["Name"].GetString() << std::endl;

		std::string input;
		do {	
			if (!input.empty())
				std::cout << "Invalid input!" << std::endl;
			std::cout << "Choose a package to download: ";
			std::cin >> input;
		} while (!is_number(input) || std::stoi(input) >= resultcount);

		rapidjson::Value& selected = doc["results"][std::stoi(input)];
		if (selected["URL"].IsString())
			return (std::string(selected["URL"].GetString()) + ".git");
		else
			return ("https://aur.archlinux.org" + std::string(selected["URLPath"].GetString()));
	}

	return "";
}

// Returns an error message instead of a package name if an error occurs
// status will be set to -1 in the case of an error as well.
std::string TaurBackend::taur_search_aur(std::string query, int *status) {
	// link to AUR API
	cpr::Url url(("https://aur.archlinux.org/rpc/v5/search/" + cpr::util::urlEncode(query) + "?by=name"));
	cpr::Response r = cpr::Get(url);
	
	std::string raw_text_response = r.text;

	rapidjson::Document json_response;
	json_response.Parse(raw_text_response.c_str());

	if (strcmp(json_response["type"].GetString(), "error") == 0) {
		*status = -1;
		return std::string(json_response["error"].GetString());
	}

	if (json_response["resultcount"].GetInt() == 0) {
		*status = -2;
		return "Package not found";
	}

	*status = 0;

	return getURL(json_response);
}