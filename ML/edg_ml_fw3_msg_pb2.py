# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: edg-ml-fw3-msg.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='edg-ml-fw3-msg.proto',
  package='edgmlfw3msg',
  syntax='proto2',
  serialized_pb=_b('\n\x14\x65\x64g-ml-fw3-msg.proto\x12\x0b\x65\x64gmlfw3msg\"\xa6\x01\n\x04Node\x12\n\n\x02id\x18\x01 \x02(\t\x12\x0f\n\x07\x65xt_ft1\x18\x02 \x01(\x01\x12\x0f\n\x07\x65xt_ft2\x18\x03 \x01(\x01\x12%\n\x05\x66\x61\x63\x65s\x18\x04 \x03(\x0b\x32\x16.edgmlfw3msg.Node.Face\x12\x0c\n\x04\x64one\x18\x05 \x01(\x08\x1a;\n\x04\x46\x61\x63\x65\x12\x10\n\x08\x66\x65\x61ture1\x18\x01 \x01(\x01\x12\x10\n\x08\x66\x65\x61ture2\x18\x02 \x01(\x01\x12\x0f\n\x07\x66\x61\x63\x65_id\x18\x03 \x01(\x05\"\x17\n\x05\x41gent\x12\x0e\n\x06status\x18\x01 \x02(\x05')
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)




_NODE_FACE = _descriptor.Descriptor(
  name='Face',
  full_name='edgmlfw3msg.Node.Face',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='feature1', full_name='edgmlfw3msg.Node.Face.feature1', index=0,
      number=1, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='feature2', full_name='edgmlfw3msg.Node.Face.feature2', index=1,
      number=2, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='face_id', full_name='edgmlfw3msg.Node.Face.face_id', index=2,
      number=3, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=145,
  serialized_end=204,
)

_NODE = _descriptor.Descriptor(
  name='Node',
  full_name='edgmlfw3msg.Node',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='id', full_name='edgmlfw3msg.Node.id', index=0,
      number=1, type=9, cpp_type=9, label=2,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='ext_ft1', full_name='edgmlfw3msg.Node.ext_ft1', index=1,
      number=2, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='ext_ft2', full_name='edgmlfw3msg.Node.ext_ft2', index=2,
      number=3, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='faces', full_name='edgmlfw3msg.Node.faces', index=3,
      number=4, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
    _descriptor.FieldDescriptor(
      name='done', full_name='edgmlfw3msg.Node.done', index=4,
      number=5, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[_NODE_FACE, ],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=38,
  serialized_end=204,
)


_AGENT = _descriptor.Descriptor(
  name='Agent',
  full_name='edgmlfw3msg.Agent',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='status', full_name='edgmlfw3msg.Agent.status', index=0,
      number=1, type=5, cpp_type=1, label=2,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=206,
  serialized_end=229,
)

_NODE_FACE.containing_type = _NODE
_NODE.fields_by_name['faces'].message_type = _NODE_FACE
DESCRIPTOR.message_types_by_name['Node'] = _NODE
DESCRIPTOR.message_types_by_name['Agent'] = _AGENT

Node = _reflection.GeneratedProtocolMessageType('Node', (_message.Message,), dict(

  Face = _reflection.GeneratedProtocolMessageType('Face', (_message.Message,), dict(
    DESCRIPTOR = _NODE_FACE,
    __module__ = 'edg_ml_fw3_msg_pb2'
    # @@protoc_insertion_point(class_scope:edgmlfw3msg.Node.Face)
    ))
  ,
  DESCRIPTOR = _NODE,
  __module__ = 'edg_ml_fw3_msg_pb2'
  # @@protoc_insertion_point(class_scope:edgmlfw3msg.Node)
  ))
_sym_db.RegisterMessage(Node)
_sym_db.RegisterMessage(Node.Face)

Agent = _reflection.GeneratedProtocolMessageType('Agent', (_message.Message,), dict(
  DESCRIPTOR = _AGENT,
  __module__ = 'edg_ml_fw3_msg_pb2'
  # @@protoc_insertion_point(class_scope:edgmlfw3msg.Agent)
  ))
_sym_db.RegisterMessage(Agent)


# @@protoc_insertion_point(module_scope)
