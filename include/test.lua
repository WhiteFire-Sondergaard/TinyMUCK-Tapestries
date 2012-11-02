@edit my_lua
1 9999 d
i
print("Command: ", command)
print("Args:    ", args)
f, e = load(args, command)
if (not f)
  print("Code failed to compile: ", e)
else
  pcall(f)
end
.
c
q
