# Havok type generation

The generator sorts complete-definition dependencies before emitting classes:
bases, embedded fields, fixed arrays, aliases, and template arguments precede
the types that use them. Pointer and dynamic-array targets can use forward
declarations, allowing recursive structures. Actual by-value/inheritance cycles
produce an error with the dependency path. Output order is deterministic.

`Basic` is not a C++ representation by itself. Generation distinguishes:

- Integer, floating-point, and boolean metadata: fixed-width scalar aliases;
  integer signedness comes from the tag format. Reads use scalar storage.
- Schema types with members: generated records with `read` and `to_json` methods.
- Known support wrappers: existing support-header implementations, including
  `hkBaseObject` and `hkFreeListArrayElement`; these are not read as POD scalars.
- `hkEnum`/`hkFlags`: one enum identity shared by wrappers, with each wrapper
  retaining its own storage type. The schema does not supply enumerator names.
- Unknown opaque types: byte-preserving objects using the schema size. This
  preserves data without inventing scalar or field semantics.

Generated template classes use explicit specializations. Matrix types preserve
their element argument even when tagged as arrays; if the schema omits that
argument, generation selects float32/float64 from their storage size.
The tag parser's invalid index-zero sentinel has no C++ definition.

The forward header also contains the complete dependency closure of `hkAabb`,
`hkUint32`, and `hkStringPtr`, which the existing extra support header embeds.

Build `ApexHavokTypegenFixture` to compile generated regression schemas, and run
CTest `Apex.HavokTypegen` to check ordering, recursion, aliases, enum identity,
repeatability, and cycle diagnostics. Existing support methods that throw
“not implemented” remain outside the generator's scope.
