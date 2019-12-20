call main
end

equal:
pop dx
je eq1
jne eq0
eq1:
push 1
jmp eq_end
eq0:
push 0
eq_end:
pop ax
pop bx
pop bx
push ax
push dx
ret

below:
pop dx
jb b1
jae b0
b1:
push 1
jmp b_end
b0:
push 0
b_end:
pop ax
pop bx
pop bx
push ax
push dx
ret

above:
pop dx
ja a1
jbe a0
a1:
push 1
jmp a_end
a0:
push 0
a_end:
pop ax
pop bx
pop bx
push ax
push dx
ret

main:
in
push [0]
pop cx
pop [cx+1]
push [0]
pop cx
push [cx+1]
out
pop [cx+1]
push [0]
pop cx
push [cx+1]
push [0]
push 2
add
pop [0]
call factorial
push 2
push [0]
sub
pop [0]
push [0]
pop cx
pop [cx+1]
push [0]
pop cx
push [cx+1]
out
pop [cx+1]
ret

factorial:
pop dx
push [0]
pop cx
pop [cx+1]
push dx
push 1
push [0]
pop cx
push [cx+1]
call equal
push 0
jae if0false
if0true:
pop dx
push [0]
pop cx
push [cx+1]
push dx
ret
jmp if0end
if0false:
push [0]
pop cx
push [cx+1]
out
pop [cx+1]
push 1
push [0]
pop cx
push [cx+1]
sub
push [0]
pop cx
pop [cx+2]
push [0]
pop cx
push [cx+2]
push [0]
push 3
add
pop [0]
call factorial
push 3
push [0]
sub
pop [0]
push [0]
pop cx
pop [cx+2]
push [0]
pop cx
push [cx+1]
push [0]
pop cx
push [cx+2]
mul
push [0]
pop cx
pop [cx+1]
pop dx
push [0]
pop cx
push [cx+1]
push dx
ret
if0end:
ret

