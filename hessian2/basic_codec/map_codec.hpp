#pragma once

#include "hessian2/codec.hpp"
#include "hessian2/object.hpp"
#include "hessian2/basic_codec/object_codec.hpp"
#include "hessian2/basic_codec/number_codec.hpp"
#include "hessian2/basic_codec/string_codec.hpp"
#include "hessian2/basic_codec/type_ref_codec.hpp"

namespace hessian2 {

//# map/object
// ::= 'M' type (value value)* 'Z'  # key, value map pairs
template <>
std::unique_ptr<TypedMapObject> Decoder::decode() {
  std::string type;
  Object::TypedMap obj_map;

  auto result = std::make_unique<TypedMapObject>();
  values_ref_.push_back(result.get());
  auto ret = reader_->Read<uint8_t>();
  ABSL_ASSERT(ret.first);
  auto code = ret.second;
  ABSL_ASSERT(code == 'M');
  // Read Type
  auto type_str = decode<Object::TypeRef>();
  if (!type_str) {
    return nullptr;
  }

  obj_map.type_name_ = std::move(type_str->type_);

  ret = reader_->Peek<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }

  while (ret.second != 'Z') {
    auto key = decode<Object>();
    if (!key) {
      return nullptr;
    }

    auto value = decode<Object>();
    if (!value) {
      return nullptr;
    }

    obj_map.field_name_and_value_.emplace(std::move(key), std::move(value));
    ret = reader_->Peek<uint8_t>();
    if (!ret.first) {
      return nullptr;
    }
  }

  // Skip last 'Z'
  reader_->Read<uint8_t>();
  result->setTypedMap(std::move(obj_map));
  return result;
}

// ::= 'H' (value value)* 'Z'       # untyped key, value
template <>
std::unique_ptr<UntypedMapObject> Decoder::decode() {
  std::string type;
  Object::UntypedMap obj_map;

  auto result = std::make_unique<UntypedMapObject>();
  values_ref_.push_back(result.get());
  auto ret = reader_->Read<uint8_t>();
  ABSL_ASSERT(ret.first);
  auto code = ret.second;
  ABSL_ASSERT(code == 'H');

  ret = reader_->Peek<uint8_t>();
  if (!ret.first) {
    return nullptr;
  }

  while (ret.second != 'Z') {
    auto key = decode<Object>();
    if (!key) {
      return nullptr;
    }

    auto value = decode<Object>();
    if (!value) {
      return nullptr;
    }

    obj_map.emplace(std::move(key), std::move(value));
    ret = reader_->Peek<uint8_t>();
    if (!ret.first) {
      return nullptr;
    }
  }

  // Skip last 'Z'
  reader_->Read<uint8_t>();
  result->setUntypedMap(std::move(obj_map));
  return result;
}

template <>
bool Encoder::encode(const TypedMapObject& value) {
  values_ref_.emplace(&value, values_ref_.size());
  auto typed_map = value.to_typed_map();
  ABSL_ASSERT(typed_map.has_value());
  auto typed_map_value = typed_map.value();
  ABSL_ASSERT(typed_map_value != nullptr);
  writer_->WriteByte('M');
  Object::TypeRef type_ref(typed_map_value->type_name_);
  encode<Object::TypeRef>(type_ref);
  for (const auto& elem : typed_map_value->field_name_and_value_) {
    encode<Object>(*elem.first);
    encode<Object>(*elem.second);
  }
  writer_->WriteByte('Z');
  return true;
}

template <>
bool Encoder::encode(const UntypedMapObject& value) {
  values_ref_.emplace(&value, values_ref_.size());
  auto untyped_map = value.to_untyped_map();
  ABSL_ASSERT(untyped_map.has_value());
  auto untyped_map_value = untyped_map.value();
  ABSL_ASSERT(untyped_map_value != nullptr);
  writer_->WriteByte('H');
  for (const auto& elem : *untyped_map_value) {
    encode<Object>(*elem.first);
    encode<Object>(*elem.second);
  }
  writer_->WriteByte('Z');
  return true;
}

}  // namespace hessian2
