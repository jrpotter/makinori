_G.nori = {}

function nori.anchor_string_match(path, pattern)
  if pattern:sub(1, 1) ~= '^' then
    pattern = '^' .. pattern
  end

  -- Checking the trailing anchor is more complicated. Check if a `$` is at the
  -- end of the pattern and match any number of `%`s that may be preceding it.
  -- An odd number of `%`s indicate the trailing `$` is escaped. Thus an even
  -- sized suffix needs the anchor.
  local trailing = pattern:match('%%*%$$')
  if trailing == nil or #trailing % 2 == 0 then
    pattern = pattern .. '$'
  end

  return path:match(pattern)
end
