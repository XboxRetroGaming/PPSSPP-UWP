#pragma once

#include <string>
#include <functional>
#include <memory>

#include "Common/File/Path.h"
#include "Common/Net/NetBuffer.h"

namespace http {

enum class RequestMethod {
	GET,
	POST,
};

enum class ProgressBarMode {
	NONE,
	VISIBLE,
	DELAYED,
};

// Abstract request.
class Download {
public:
	Download(const std::string &url, const std::string &name, bool *cancelled) : url_(url), name_(name), progress_(cancelled) {}
	virtual ~Download() {}

	virtual void SetAccept(const char *mime) = 0;
	virtual void SetUserAgent(const std::string &userAgent) = 0;

	// NOTE: Completion callbacks (which these are) are deferred until RunCallback is called. This is so that
	// the call will end up on the thread that calls g_DownloadManager.Update().
	void SetCallback(std::function<void(Download &)> callback) {
		callback_ = callback;
	}
	void RunCallback() {
		if (callback_) {
			callback_(*this);
		}
	}

	virtual void Start() = 0;
	virtual void Join() = 0;

	virtual bool Done() const = 0;
	virtual bool Failed() const = 0;

	virtual int ResultCode() const = 0;

	// Returns 1.0 when done. That one value can be compared exactly - or just use Done().
	float Progress() const { return progress_.progress; }
	float SpeedKBps() const { return progress_.kBps; }
	std::string url() const { return url_; }
	virtual const Path &outfile() const = 0;

	virtual void Cancel() = 0;
	virtual bool IsCancelled() const = 0;

	// Response
	virtual Buffer &buffer() = 0;
	virtual const Buffer &buffer() const = 0;

protected:
	std::function<void(Download &)> callback_;
	std::string url_;
	std::string name_;
	net::RequestProgress progress_;

private:
};

using std::shared_ptr;

class RequestManager {
public:
	~RequestManager() {
		CancelAll();
	}

	std::shared_ptr<Download> StartDownload(const std::string &url, const Path &outfile, ProgressBarMode mode, const char *acceptMime = nullptr);

	std::shared_ptr<Download> StartDownloadWithCallback(
		const std::string &url,
		const Path &outfile,
		ProgressBarMode mode,
		std::function<void(Download &)> callback,
		const std::string &name = "",
		const char *acceptMime = nullptr);

	std::shared_ptr<Download> AsyncPostWithCallback(
		const std::string &url,
		const std::string &postData,
		const std::string &postMime, // Use postMime = "application/x-www-form-urlencoded" for standard form-style posts, such as used by retroachievements. For encoding form data manually we have MultipartFormDataEncoder.
		ProgressBarMode mode,
		std::function<void(Download &)> callback,
		const std::string &name = "");

	// Drops finished downloads from the list.
	void Update();
	void CancelAll();

	void WaitForAll();
	void SetUserAgent(const std::string &userAgent) {
		userAgent_ = userAgent;
	}

private:
	std::vector<std::shared_ptr<Download>> downloads_;
	// These get copied to downloads_ in Update(). It's so that callbacks can add new downloads
	// while running.
	std::vector<std::shared_ptr<Download>> newDownloads_;

	std::string userAgent_;
};

}  // namespace net
