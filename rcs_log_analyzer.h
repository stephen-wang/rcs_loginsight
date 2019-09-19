#pragma once
#include <vector>
#include <unordered_map>


// Test if nth bit of num is set
#define TEST_BIT(num, nth) ((num >> nth) & 0x01)

// line tokens
#define TOKEN_AGV_STATUS_LINE "agv status	line_"
#define TOKEN_DUP_AGV_STATUS_LINE "agv status: same as before"
#define TOKEN_AGV_CONNECTION_LOST "lost_connection"

// field tokens
//// for parsing AGV status report
#define TOKEN_TIMESTAMP_START		" "
#define TOKEN_TIMESTAMP_END		" "
#define TOKEN_AGV_ID_START		"agv"
#define TOKEN_AGV_ID_END		":"
#define TOKEN_MD_CODE_START		"md_code="
#define TOKEN_MD_CODE_END		","
#define TOKEN_OPTO_IN_STATUS_B0_START	"opto_in_status_b0="
#define TOKEN_OPTO_IN_STATUS_B0_END	","
#define TOKEN_OPTO_OUT_STATUS_B1_START	"opto_out_status_b1="
#define TOKEN_OPTO_OUT_STATUS_B1_END	","


class RcsLogInsightUtil {
public:
	RcsLogInsightUtil()=delete;
	~RcsLogInsightUtil()=delete;
	static std::string get_field_from_str(const std::string &, const char*, int,
		const char *, int);
	static std::string get_log_timestamp(const std::string &);
	static std::string get_agv_id(const std::string &);
	static std::string formatted_millisec(unsigned long duration);
};


class Timestamp {
public:
	Timestamp(const std::string &);
	long diff(const Timestamp&);

private:
	int day_=0;
	int hour_=0;
	int min_=0;
	int sec_=0;
	int ms_=0;
};


// Data structure for describing AGV status on some DM code
class AgvDmInfo{
public:
	AgvDmInfo() = default;
	AgvDmInfo(const AgvDmInfo&)=default;
	AgvDmInfo(const std::string &);

	void parse(const std::string&);
	std::string get_md_code(const std::string &);
	bool get_nth_bit_of_opto_in_status_b0(const std::string&, int);
	bool get_nth_bit_of_opto_out_status_b1(const std::string&, int);
	bool get_anti_collision(const std::string &);
	bool get_current_overload(const std::string &);
	bool get_force_stop(const std::string &);
	bool get_move_engine_stuck(const std::string &);
	bool get_left_right_wheels_diff(const std::string &);
	bool get_dm_code_not_found(const std::string &);
	void set_leave_time(const std::string &leave);
	void compute_duration();
	std::string to_string();
	bool operator < (const AgvDmInfo&) const;
	void update(const AgvDmInfo&);

public:
	std::string agv_id_ = "null";
	std::string enter_ = "";		// timestamp to reach current DM code
	std::string leave_ = "";		// timestamp to leave current DM code
	unsigned long duration_ = 0;		// how long (in milliseconds) AGV stayed on current
	                                        // DM code
	std::string md_code_ = "0";		// value of current DM code
	bool anti_collision_ = false;		// If anti_collision is triggered
	bool current_overload_ = false;		// If current is overloaded
	bool force_stop_ = false;		// If force_stop is manually triggered
	bool move_engine_stuck_ = false; 	// If the engine of left/right wheels is stuck
	bool left_right_wheels_diff_ = false;	// If the calibration difference between left and right wheels
	                                        // is too big
	bool dm_code_not_found_ = false;	// If DM code is not found
	int opto_in_status_b0_ = -1;
	int opto_out_status_b1_ = -1;
};


struct AgvLostInfo {
	std::string when_="";       // timestamp of connection lost
	std::string agv_id_="null";
};


struct RebootInfo {
	std::string when_="";       // timestamp of reboot
	std::string agv_id_="null";
};


struct PostRequestInfo {
	std::string when_;
	std::string mission_id_;
	std::string mission_list_;
};


enum LogItemType {
	LOG_TYPE_AGV_STATUS_REPORT= 0,
	LOG_TYPE_DUP_AGV_STATUS_REPORT,
	LOG_TYPE_CONNECTION_LOST,
	LOG_TYPE_REBOOT,
	LOG_TYPE_POST_REQUETA,
	LOG_TYPE_MISSIOIN_CANCEL,
	LOG_TYPE_NULL,
};


struct AnalysisResult {
	long total_lines;
	std::string duration;
	CString result_file;
};


// Callback to update progress
typedef void(prog_cb)(long, const AnalysisResult&);

class CRcsLogAnalyzer
{
public:
	CRcsLogAnalyzer(const CString& log_file_path)
		: log_file_path_(log_file_path), result_file_path_(log_file_path) {
		result_file_path_.Append(CString(".result"));
	}

	~CRcsLogAnalyzer() = default;
	static void do_work(const CString &log_file_path, prog_cb update);
	static void stop_work() { stop_work_ = true; }

private:
	LogItemType analyze_log(const std::string &, AgvDmInfo &, AgvLostInfo &);

	CString log_file_path_;
	CString result_file_path_;
	static bool stop_work_;
};

