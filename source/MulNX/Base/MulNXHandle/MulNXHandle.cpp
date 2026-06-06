#include "MulNXHandle.hpp"

MulNXHandle::MulNXHandle() {
    this->Value = MulNXHandle::Invalid;
}
MulNXHandle MulNXHandle::CreateHandle() {
    MulNXHandle handle{};
    handle.Value = MulNXHandle::CurrentHandleValue.fetch_add(1);
    return handle;
}
bool MulNXHandle::IsValid()const {
    return this->Value != MulNXHandle::Invalid;
}
uint64_t MulNXHandle::GetValue()const {
    return this->Value;
}