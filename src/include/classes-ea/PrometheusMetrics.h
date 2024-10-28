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
#include "../classes/Std.h"

/**
 * Stores information about Prometheus schema group.
 */
class PrometheusSchemaField : public Dynamic {
 public:
  /* Public fields */

  // Name of the field.
  string name;

  // Type of the field.
  ENUM_DATATYPE type;

  // Description of the field.
  string help;

  /**
   * Constructor.
   */
  PrometheusSchemaField(string _name, ENUM_DATATYPE _type, string _help = "") {
    name = _name;
    type = _type;
    help = _help;
  }
};

/**
 * Stores information about Prometheus schema group.
 */
class PrometheusSchemaGroup : public Dynamic {
 public:
  /* Protected fields */

  // Name of the group.
  string name;

  // Type of the group: counter, gauge, histogram, summary.
  string type;

  // Description of the group.
  string help;

  // Dict of field name => field information.
  DictStruct<string, Ref<PrometheusSchemaField>> fields;

  /**
   * Constructor.
   */
  PrometheusSchemaGroup(string _name, string _type, string _help) {
    name = _name;
    type = _type;
    help = _help;
  }

  /* Public methods */

  /**
   * Adds field to this schema group.
   */
  void AddField(string _field_name, ENUM_DATATYPE _field_type, string _field_help = "") {
    if (!fields.KeyExists(_field_name)) {
      Alert("Field \"", _field_name, "\" has been already added to the group \"", name, "\"!");
      DebugBreak();
      return;
    }
    Ref<PrometheusSchemaField> _field = new PrometheusSchemaField(_field_name, _field_type, _field_help);
    fields.Set(_field_name, _field);
  }
};

/**
 * Class to create schema for Prometheus Metrics to be exported from EA.
 */
class PrometheusSchema : public Dynamic {
 protected:
  /* Protected fields */

  // Dict of group name => group information.
  DictStruct<string, Ref<PrometheusSchemaGroup>> groups;

 public:
  /* Public methods */

  /**
   * Adds field group to the schema.
   */
  void AddGroup(string name, string type, string help = "") {
    Ref<PrometheusSchemaGroup> _group = new PrometheusSchemaGroup(name, type, help);
    groups.Set(name, _group);
  }

  /**
   * Adds field to the given schema group.
   */
  void AddField(string _group_name, string _field_name, ENUM_DATATYPE _field_type, string _field_help = "") {
    // Searching for the group.
    unsigned int _group_pos;
    if (!groups.KeyExists(_group_name, _group_pos)) {
      Alert("You have to add the group \"", _group_name, "\" via AddGroup() before using AddField()!");
      DebugBreak();
      return;
    }
    Ref<PrometheusSchemaGroup> _group = groups.GetByPos(_group_pos);
    _group.Ptr().AddField(_field_name, _field_type, _field_help);
  }
};

union TypedValueUnion {
  bool vbool;
  long vlong;
  double vdouble;
};

enum ENUM_TYPED_VALUE_TYPE {
  TYPED_VALUE_TYPE_BOOL,
  TYPED_VALUE_TYPE_INT,
  TYPED_VALUE_TYPE_LONG,
  TYPED_VALUE_TYPE_DOUBLE,
  TYPED_VALUE_TYPE_STRING
};

/**
 * Class stores a value that can set and retrieved by any type.
 */
class TypedValue {
 protected:
  /* Protected fields */

  // POD value stored.
  TypedValueUnion value;

  // String value stored.
  string vstring;

  // Date and time value stored.
  datetime vdatetime;

  // Type of value stored.
  ENUM_DATATYPE type;

 public:
  /**
   * Constructor
   */
  TypedValue(ENUM_DATATYPE _type) {
    type = _type;

    switch (type) {
      case TYPE_BOOL:
        value.vbool = false;
        break;
      case TYPE_CHAR:
        vstring = "";
        break;
      case TYPE_UCHAR:
      case TYPE_SHORT:
      case TYPE_USHORT:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
        value.vlong = 0;
        break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
        value.vdouble = 0;
        break;
      case TYPE_STRING:
        vstring = "";
        break;
      case TYPE_DATETIME:
        vdatetime = (datetime)0;
        break;
      default:
        Alert("Unsupported type: ", EnumToString(type));
        DebugBreak();
    }
  }

  /* Public methods */

  /**
   * Sets value from given boolean.
   */
  void Set(bool _value) {
    switch (type) {
      case TYPE_BOOL:
        value.vbool = _value;
        break;
      case TYPE_CHAR:
        vstring = _value ? "+" : "-";
        break;
      case TYPE_UCHAR:
      case TYPE_SHORT:
      case TYPE_USHORT:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
        value.vlong = _value;
        break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
        value.vdouble = _value ? 1.0 : 0;
        break;
      case TYPE_STRING:
        vstring = _value ? "True" : "False";
        break;
      case TYPE_DATETIME:
        Alert("Could not initialize datetime property from bool value!");
        DebugBreak();
        break;
      default:
        Alert("Unsupported type: ", EnumToString(type));
        DebugBreak();
    }
  }

  /**
   * Sets value from given number.
   */
  void Set(long _value) {
    switch (type) {
      case TYPE_BOOL:
        value.vbool = _value != 0;
        break;
      case TYPE_CHAR:
        vstring = (_value != 0) ? "+" : "-";
        break;
      case TYPE_UCHAR:
      case TYPE_SHORT:
      case TYPE_USHORT:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
        value.vlong = _value;
        break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
        value.vdouble = (double)_value;
        break;
      case TYPE_STRING:
        vstring = IntegerToString(_value);
        break;
      case TYPE_DATETIME:
        vdatetime = (datetime)_value;
        break;
      default:
        Alert("Unsupported type: ", EnumToString(type));
        DebugBreak();
    }
  }

  /**
   * Sets value from given number.
   */
  void Set(double _value) {
    switch (type) {
      case TYPE_BOOL:
        value.vbool = _value != 0;
        break;
      case TYPE_CHAR:
        vstring = (_value != 0) ? "+" : "-";
        break;
      case TYPE_UCHAR:
      case TYPE_SHORT:
      case TYPE_USHORT:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
        value.vlong = (long)_value;
        break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
        value.vdouble = _value;
        break;
      case TYPE_STRING:
        vstring = DoubleToString(_value);
        break;
      case TYPE_DATETIME:
        Alert("Could not initialize datetime property from double value!");
        DebugBreak();
        break;
      default:
        Alert("Unsupported type: ", EnumToString(type));
        DebugBreak();
    }
  }

  /**
   * Sets value from given string.
   */
  void Set(string _value) {
    switch (type) {
      case TYPE_BOOL:
        value.vbool = _value == "+" || _value == "True" || _value == "true" || _value == "Yes" || _value == "yes" ||
                      _value == "On" || _value == "on" || _value == "Enabled" || _value == "enabled";
        break;
      case TYPE_CHAR:
        vstring = StringSubstr(_value, 0, 1);
        break;
      case TYPE_UCHAR:
      case TYPE_SHORT:
      case TYPE_USHORT:
      case TYPE_INT:
      case TYPE_UINT:
      case TYPE_LONG:
      case TYPE_ULONG:
        value.vlong = StringToInteger(_value);
        break;
      case TYPE_FLOAT:
      case TYPE_DOUBLE:
        value.vdouble = StringToDouble(_value);
        break;
      case TYPE_STRING:
        vstring = _value;
        break;
      case TYPE_DATETIME:
        vdatetime = StringToTime(_value);
        break;
      default:
        Alert("Unsupported type: ", EnumToString(type));
        DebugBreak();
    }
  }
};

/**
 * Structure stores value for the Prometheus metrics field.
 */
struct PrometheusMetricsValue {
  // Name of the group.
  string group_name;

  // Name of the field.
  string field_name;

  // Value for the field.
  TypedValue value;

  /**
   * Default constructor.
   */
  PrometheusMetricsValue() : value(TYPE_INT) {}

  /**
   * Constructor.
   */
  PrometheusMetricsValue(string _group_name, string _field_name, ENUM_DATATYPE _field_type) : value(_field_type) {
    group_name = _group_name;
    field_name = _field_name;
  }
};

/**
 * Class to export Prometheus Metrics from EA.
 */
class PrometheusMetrics : public Dynamic {
 protected:
  /* Protected fields */

  // Reference to Prometheus fields schema.
  Ref<PrometheusSchema> schema;

  // Map of added values. Key is a group name + field name. Keys must be unique.
  DictStruct<string, PrometheusMetricsValue> values;

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

  /* Public methods */

  /**
   * Prepares for a new set of values.
   */
  void Clear() {
    // @todo
  }

  /**
   * Outputs values as Prometheus-compatible code.
   */
  string ToString() {
    // @todo
    return "";
  }

  /**
   * Checks if we can set given field to given type of value.
   */
  ENUM_DATATYPE CheckSchemaFieldCompatibility(string _group_name, string _field_name, ENUM_DATATYPE _type) {
    // @todo
    return TYPE_INT;
  }

  /**
   * Sets value of the given field.
   */
  void SetValue(string _group_name, string _field_name, bool value) {
    ENUM_DATATYPE _field_type = CheckSchemaFieldCompatibility(_group_name, _field_name, TYPE_BOOL);

    PrometheusMetricsValue _value(_group_name, _field_name, _field_type);
    _value.value.Set(value);

    values.Set(_group_name + ":" + _field_name, _value);
  }
};
