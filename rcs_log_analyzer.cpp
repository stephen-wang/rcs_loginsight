#include "stdafx.h"
#include "afx.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "rcs_log_analyzer.h"


// Parse timestamp in format "2019-08-26 22:27:03,887"
Timestamp::Timestamp(const std::string& ts_str)
{
	// parse day which is after the 2nd '-'
	auto day_start = ts_str.find("-");
	if (day_start == std::string::npos) {
		return;
	}

	day_start = ts_str.find("-", day_start+1);
	if (day_start == std::string::npos) {
		return;
	}

	day_start += 1;
	auto day_end = ts_str.find(" ", day_start);
	if (day_end == std::string::npos) {
		return;
	}
	day_ = std::stoi(ts_str.substr(day_start, day_end - day_start));

	// parse hour/min/sec/milli sec
	hour_ = std::stoi(RcsLogInsightUtil::get_field_from_str(ts_str, " ", 1, ":", 1));
	min_ = std::stoi(RcsLogInsightUtil::get_field_from_str(ts_str, ":", 1, ":", 1));
	sec_ = std::stoi(RcsLogInsightUtil::get_field_from_str(ts_str, ":", 2, ",", 1));

	auto ms_start = ts_str.find(",");
	if (ms_start != std::string::npos) {
		ms_start++;
		ms_ = std::stoi(ts_str.substr(ms_start));
	}
}


// Convert milliseconds into graceful string
std::string RcsLogInsightUtil::formatted_millisec(unsigned long duration) {
	std::string result = "0ms";
	
	if (duration <=0) {
		return result;
	}

	result = std::to_string(duration % 1000) + "ms";
	duration /= 1000;
	if (duration > 0) {
		result = std::to_string(duration % 60) + "s" + result;
		duration /= 60;
		if (duration > 0) {
			result = std::to_string(duration % 60) + "m" + result;
			duration /= 60;
			if (duration > 0) {
				result = std::to_string(duration) + "h" + result;
			}
		}
	}

	return result;
}


// Get time difference between two points in time based on two assumptions:
//	1) 'other' is always later than 'this' object;
//      2) the difference should be within 24hours;
long Timestamp::diff(const Timestamp &other)
{
	long result = 0;
	Timestamp cur(*this);

	result = cur.ms_ - other.ms_;
	if (other.ms_ > cur.ms_) {
		result += 1000;
		cur.sec_ -= 1;
	}

	result += (cur.sec_ - other.sec_) * 1000;
	if (other.sec_ > cur.sec_) {
		result += 60 * 1000;
		cur.min_ -= 1;
	}

	result += (cur.min_ - other.min_) * 60 * 1000;
	if (other.min_ > cur.min_) {
		result += 60 * 60 * 1000;
		cur.hour_ -= 1;
	}

	result += (cur.hour_ - other.hour_) * 60 * 60 * 1000;
	if (other.hour_ > cur.hour_) {
		result += 24 * 60 * 60 * 1000;
		cur.day_ -= 1;
	}

	return result;
}


// Parse sepcific field from log line
std::string RcsLogInsightUtil::get_field_from_str(const std::string &str,
	const char* field_start, int field_start_n, const char *field_end,
	int field_end_n)
{
	std::string value = "null";
	size_t pos_start = -1;
	while (field_start_n > 0) {
		pos_start = str.find(field_start, pos_start+1);
		if (pos_start == std::string::npos) {
			return value;
		}
		field_start_n--;
	}

	size_t pos_end = pos_start;
	while (field_end_n > 0) {
		pos_end = str.find(field_end, pos_end + 1);
		if (pos_end == std::string::npos) {
			return value;
		}
		field_end_n--;
	}

	pos_start += strlen(field_start);
	value = str.substr(pos_start, pos_end-pos_start);
	return value;
}


// Parse timestamp from log line
std::string RcsLogInsightUtil::get_log_timestamp(const std::string &log_line)
{
	auto ts_end = log_line.find(' ');
	if (ts_end == std::string::npos) {
		return "";
	}
	ts_end = log_line.find(' ', ts_end + 1);
	return log_line.substr(0, ts_end);
}


// Parse agv ID from agv status report line
std::string RcsLogInsightUtil::get_agv_id(const std::string &log_line)
{
	return RcsLogInsightUtil::get_field_from_str(log_line, TOKEN_AGV_ID_START, 1,
		TOKEN_AGV_ID_END, 1);
}


AgvDmInfo::AgvDmInfo(const std::string &agv_status_str)
{
	parse(agv_status_str);
}


void AgvDmInfo::parse(const std::string& agv_status_str)
{
	opto_in_status_b0_ = -1;
	opto_out_status_b1_ = -1;
	agv_id_ = RcsLogInsightUtil::get_agv_id(agv_status_str);
	enter_ = leave_ = RcsLogInsightUtil::get_log_timestamp(agv_status_str);
	md_code_ = get_md_code(agv_status_str);
	anti_collision_ = get_anti_collision(agv_status_str);
	current_overload_ = get_current_overload(agv_status_str);
	force_stop_ = get_force_stop(agv_status_str);
	move_engine_stuck_ =  get_move_engine_stuck(agv_status_str);
	left_right_wheels_diff_ = get_left_right_wheels_diff(agv_status_str);
	dm_code_not_found_ = get_dm_code_not_found(agv_status_str);
}


// Parse MD code from agv status report line
std::string AgvDmInfo::get_md_code(const std::string &log_line)
{
	return RcsLogInsightUtil::get_field_from_str(log_line, TOKEN_MD_CODE_START, 1,
		TOKEN_MD_CODE_END, 1);
}


// Get the nth bit of opto_in_status_b0
bool AgvDmInfo::get_nth_bit_of_opto_in_status_b0(const std::string& log_line, int n)
{
	if (opto_in_status_b0_ == -1) {
		auto str_opto_in_status_b0 = RcsLogInsightUtil::get_field_from_str(
			log_line, TOKEN_OPTO_IN_STATUS_B0_START, 1, TOKEN_OPTO_IN_STATUS_B0_END, 1);

		opto_in_status_b0_ = std::atoi(str_opto_in_status_b0.c_str());
	}
	return TEST_BIT(opto_in_status_b0_, n);
}


// Parse anti-collision state (the 0th bit of opto_in_status_b0)
bool AgvDmInfo::get_anti_collision(const std::string& log_line)
{
	return (get_nth_bit_of_opto_in_status_b0(log_line, 0)
	        || get_nth_bit_of_opto_in_status_b0(log_line, 1));
}


// Get the nth bit of opto_out_status_b1
bool AgvDmInfo::get_nth_bit_of_opto_out_status_b1(const std::string& log_line, int n)
{
	if (opto_out_status_b1_ == -1) {
		auto str_opto_out_status_b1 = RcsLogInsightUtil::get_field_from_str(
			log_line, TOKEN_OPTO_OUT_STATUS_B1_START, 1, TOKEN_OPTO_OUT_STATUS_B1_END, 1);

		opto_out_status_b1_ = std::atoi(str_opto_out_status_b1.c_str());
	}
	return TEST_BIT(opto_out_status_b1_, n);
}


// Parse current overload state (the 0th bit of opto_out_status_b1)
bool AgvDmInfo::get_current_overload(const std::string& log_line)
{
	return get_nth_bit_of_opto_out_status_b1(log_line, 0);
}


// Parse force stop state (the 4th bit of opto_out_status_b1)
bool AgvDmInfo::get_force_stop(const std::string& log_line)
{
	return get_nth_bit_of_opto_out_status_b1(log_line, 4);
}


// Parse move engine stuck state (the 5th bit of opto_out_status_b1)
bool AgvDmInfo::get_move_engine_stuck(const std::string& log_line)
{
	return get_nth_bit_of_opto_out_status_b1(log_line, 5);
}


// Parse difference state between left & right wheels (the 6th bit of opto_out_status_b1)
bool AgvDmInfo::get_left_right_wheels_diff(const std::string& log_line)
{
	return get_nth_bit_of_opto_out_status_b1(log_line, 6);
}


// Parse "DM code not found" field (the 7th bit of opto_out_status_b1)
bool AgvDmInfo::get_dm_code_not_found(const std::string& log_line)
{
	return get_nth_bit_of_opto_out_status_b1(log_line, 7);
}


void AgvDmInfo::set_leave_time(const std::string &leave)
{
	if (strcmp(leave.c_str(), leave_.c_str()) > 0) {
		leave_ = leave;
	}
}


void AgvDmInfo::compute_duration()
{
	Timestamp begin(enter_);
	Timestamp end(leave_);
	duration_ = end.diff(begin);
}


std::string AgvDmInfo::to_string() {
	if (duration_ == 0) {
		compute_duration();
	}

	std::ostringstream oss;
	oss << md_code_ << ": Start " << enter_ << ", duration ";
	oss << RcsLogInsightUtil::formatted_millisec(duration_);
	if (anti_collision_) {
		oss << ", anti collision " << anti_collision_;
	}

	if (force_stop_) {
		oss << ", force stop " << force_stop_;
	}

	if (move_engine_stuck_) {
		oss << ", move engine stuck " << move_engine_stuck_;
	}

	if (left_right_wheels_diff_) {
		oss << ", L/R wheels diff " << left_right_wheels_diff_;
	}

	if (dm_code_not_found_) {
		oss << ", DM code not found " << dm_code_not_found_;
	}

	return oss.str();
}


bool AgvDmInfo::operator < (const AgvDmInfo &other) const
{
	return (duration_ < other.duration_);
}


void AgvDmInfo::update(const AgvDmInfo& other)
{
	if (other.anti_collision_ && !anti_collision_) {
		anti_collision_ = true;
	}

	if (other.current_overload_ && !current_overload_) {
		current_overload_ = true;
	}

	if (other.force_stop_ && ! force_stop_) {
		force_stop_ = true;
	}

	if (other.move_engine_stuck_ && !move_engine_stuck_) {
		move_engine_stuck_ = true;
	}

	if (other.left_right_wheels_diff_ && !left_right_wheels_diff_) {
		left_right_wheels_diff_ = true;
	}

	if (other.dm_code_not_found_ && !dm_code_not_found_) {
		dm_code_not_found_ = true;
	}

	set_leave_time(other.leave_);
}


bool CRcsLogAnalyzer::stop_work_ = false;


// Analyze one log line
LogItemType CRcsLogAnalyzer::analyze_log(const std::string &log_line,
	AgvDmInfo &adi, AgvLostInfo &cli)
{
	auto token_pos = log_line.find(TOKEN_AGV_STATUS_LINE);
	if (token_pos != std::string::npos) {
		adi.parse(log_line);
		return LOG_TYPE_AGV_STATUS_REPORT;
	}

	token_pos = log_line.find(TOKEN_DUP_AGV_STATUS_LINE);
	if (token_pos != std::string::npos) {
		adi.enter_ = adi.leave_ = RcsLogInsightUtil::get_log_timestamp(log_line);
		adi.agv_id_ = RcsLogInsightUtil::get_agv_id(log_line);
		return LOG_TYPE_DUP_AGV_STATUS_REPORT;
	}

	token_pos = log_line.find(TOKEN_AGV_CONNECTION_LOST);
	if (token_pos != std::string::npos) {
		cli.when_ = RcsLogInsightUtil::get_log_timestamp(log_line);
		cli.agv_id_ = RcsLogInsightUtil::get_agv_id(log_line);
		return LOG_TYPE_CONNECTION_LOST;
	}

	return LOG_TYPE_NULL;
}


//
// Entrance to RCS log analysis
//
void CRcsLogAnalyzer::do_work(const CString& log_file_path, prog_cb update_fn)
{
	if (log_file_path == "") {
		AfxMessageBox(CString("Log file is empty"));
		return;
	}

	CRcsLogAnalyzer executor(log_file_path);
	executor.log_file_path_ = log_file_path;
	std::string err_info("Failed to anayze log ");
	err_info += executor.log_file_path_ + ": ";
	try {
		auto time_start = std::chrono::steady_clock::now();
		int line_no = 0;
		std::ifstream in(executor.log_file_path_.GetString());
		std::ofstream out(executor.result_file_path_.GetString());
		std::unordered_map<std::string, std::vector<AgvDmInfo>> agv_dm_info;
		std::unordered_map<std::string, std::vector<AgvLostInfo>> agv_lost_info;

		AnalysisResult result;
		AgvDmInfo last_adi, cur_adi;
		AgvLostInfo ali;
		stop_work_ = false;
		while (!stop_work_ && !in.eof() && in.good()) {
			std::string cur_line;

			std::getline(in, cur_line);
			auto log_type = executor.analyze_log(cur_line, cur_adi, ali);
			if (log_type == LOG_TYPE_AGV_STATUS_REPORT) {
				if (agv_dm_info.find(cur_adi.agv_id_)
				    == agv_dm_info.end()) {
					agv_dm_info[cur_adi.agv_id_]
				        = std::vector<AgvDmInfo>(1, cur_adi);
				}
				else {
					auto agv_id = cur_adi.agv_id_;
					auto last_adi = agv_dm_info[agv_id].rbegin();
					if (cur_adi.md_code_ == last_adi->md_code_) {
						last_adi->update(cur_adi);
					} else {
						agv_dm_info[cur_adi.agv_id_].push_back(cur_adi);
					}
				}
			} else if( log_type == LOG_TYPE_DUP_AGV_STATUS_REPORT) {
				if (agv_dm_info.find(cur_adi.agv_id_) != agv_dm_info.end()) {
					auto last_adi = agv_dm_info[cur_adi.agv_id_].rbegin();
					last_adi->set_leave_time(cur_adi.leave_);
				}
			} else if(log_type == LOG_TYPE_CONNECTION_LOST) {
				if (agv_lost_info.find(ali.agv_id_) == agv_lost_info.end()) {
					agv_lost_info[ali.agv_id_] = std::vector<AgvLostInfo>(1, ali);
				} else {
					agv_lost_info[ali.agv_id_].push_back(ali);
				}
			}

			line_no++;
			if (line_no % 2000 == 0) {
				//TRACE(CString(("processing " + std::to_string(line_no) + "...\n").c_str()));
				update_fn(line_no, result);
			}
		}

		result.total_lines = line_no;
		result.result_file = executor.result_file_path_;

		for(auto& adi_pair: agv_dm_info) {
			auto agv_id = adi_pair.first;
			out << "AGV " << agv_id << std::endl;

			// Save connection lost info
			if (agv_lost_info.find(agv_id) != agv_lost_info.end()) {
				for (auto& ali : agv_lost_info[agv_id]) {
					out << "\t" << ali.when_ << ", connection lost" << std::endl;
				}
				agv_lost_info[agv_id].clear();
			}

			// Save DM code info
			/*
			for (auto& adi : adi_pair.second) {
				adi.compute_duration();
			}

			if (adi_pair.second.size() > 1) {
				std::sort(adi_pair.second.begin(), adi_pair.second.end());
			}
			*/
			for (auto& adi : adi_pair.second) {
				out << "\t" << adi.to_string() << std::endl;
			}
			adi_pair.second.clear();
		}

		// Compute and format analysis duration
		auto time_end = std::chrono::steady_clock::now();
		auto duration = time_end - time_start;
		using std::chrono::duration_cast;
		auto elapse_ms = duration_cast<std::chrono::milliseconds>(duration).count();
		result.duration = RcsLogInsightUtil::formatted_millisec((unsigned long)elapse_ms);

		// Notify to update UI
		update_fn(-1, result);

		// Cleanup
		agv_lost_info.clear();
		agv_dm_info.clear();
		in.close();
		out.close();
	} catch (const std::exception &e) {
		err_info += e.what();
		AfxMessageBox(CString(err_info.c_str()));
	} catch (...) {
		err_info += "unexpected error";
		AfxMessageBox(CString(err_info.c_str()));
	}
}
