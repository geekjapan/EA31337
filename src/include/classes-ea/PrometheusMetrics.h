//+------------------------------------------------------------------+
//|                                                          EA31337 |
//|                                 Copyright 2016-2024, EA31337 Ltd |
//|                                       https://ea31337.github.io/ |
//+------------------------------------------------------------------+

/*
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file
 * Class to export Prometheus Metrics from EA.
 *
 * @see: https://prometheus.io/docs/instrumenting/exposition_formats/
 */

/**
 * @todo
 * The following metrics can be implemented:
 *
 * # HELP ...
 * # TYPE account Account
 * account_balance
 * account_credit
 * account_equity
 * account_margin
 * account_margin_free
 * account_margin_level
 * account_profit
 * @see: https://www.mql5.com/en/docs/constants/environment_state/accountinformation
 *
 *
 * # HELP ...
 * # TYPE ea EA
 * ea_risk_margin_max # From EAParams
 * ea_stg_processed_periods # From OnTick's _result.stg_processed_periods
 * ea_total_strategies_active # Or maybe it can be aggregated as sum from the other metrics.
 *
 * # HELP ...
 * # TYPE order Order
 * order_ # How we can list all active orders?
 *
 * # HELP ...
 * # TYPE trade Trade
 * trade_lot_size{strategy="", tf=""} # From StgParams
 * trade_magic_no{strategy="", tf=""}
 * trade_max_spread{strategy="", tf=""}
 *
 * # HELP ...
 * # TYPE strategy Strategy
 * strategy_params_id{strategy="", tf=""} 0.01 # From StgParams via Get()
 * strategy_params_lot_size{strategy="", tf=""} 0.01 # From StgParams via Get()
 *
 * # HELP ... Current symbol information.
 * # TYPE symbol SymbolInfo
 * symbolinfo_tick{symbol="EURUSD"} price? (https://www.mql5.com/en/docs/marketinformation/symbolinfotick ?)
 *
 * @see: https://prometheus.io/docs/instrumenting/exposition_formats/
 *
 * # HELP ...
 * # TYPE terminal Terminal
 * terminal_cpu_cores
 * terminal_datetime_time_current (@see: https://www.mql5.com/en/docs/dateandtime)
 * terminal_datetime_time_local
 * terminal_datetime_time_trade_server
 * terminal_disk_space
 * terminal_last_error
 * terminal_memory_available
 * terminal_memory_physical
 * terminal_memory_total
 * terminal_memory_used
 * @see: https://www.mql5.com/en/book/common/environment/env_resources
 * terminal_conneted
 * terminal_ping_last
 * @see: https://www.mql5.com/en/book/common/environment/env_connectivity
 */

// Includes.
#include "../classes/File.mqh"

/**
 * Class to export Prometheus Metrics from EA.
 */
class PrometheusMetrics {
 protected:
  /* Protected methods */

  /**
   * Init code (called on constructor).
   */
  void Init() {}

 public:
  /**
   * Class constructor.
   */
  PrometheusMetrics() { Init(); }

  /**
   * Class deconstructor.
   */
  ~PrometheusMetrics() {}

  // @todo : Method to export metrics into the file using File class.
};
