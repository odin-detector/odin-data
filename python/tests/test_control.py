from odin_data.control.odin_data_controller import splice_params_metadata

# class TestControl:
#     def test_splice_params_metadata(self):
#         """Verify that leaf node values are successfully paired with their matching metadata path."""
#         # 1. Arrange input parameters matching the target structure
#         params = {
#             "param1": "values",
#             "param2": 274,
#             "info": {
#                 "phoneNumber": 2026,
#                 "state": True,
#                 "name": {"first": "odin", "last": "data"},
#                 "dob": 2011,
#             },
#         }

#         # 2. Arrange metadata mirroring the exact same keys
#         metadata = {
#             "param1": "string_type",
#             "param2":{
#                 "type": "int_type",
#                 "max": 4
#             },
#             "info": {
#                 "phoneNumber": "numeric_id",
#                 "state": "boolean_flag",
#                 "name": {"first": "string_name", "last": "string_surname"},
#                 "dob": "year_type",
#             },
#         }

#         # 3. Define the expected combined output structure (value, metadata)
#         expected_output = {
#             "param1": ("values", "string_type"),
#             "param2": (274, {"type": "int_type", "max": 4}),
#             "info": {
#                 "phoneNumber": (2026, "numeric_id"),
#                 "state": (True, "boolean_flag"),
#                 "name": {
#                     "first": ("odin", "string_name"),
#                     "last": ("data", "string_surname"),
#                 },
#                 "dob": (2011, "year_type"),
#             },
#         }

#         # 4. Act
#         result = splice_params_metadata(params, metadata)

#         # 5. Assert
#         assert result == expected_output
