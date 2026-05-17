from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Environment import Environment


def _split(env: Environment, value):
    if value is None:
        return []
    return [env.subst(str(item)) for item in env.Split(value) if env.subst(str(item))]


def _append_option(args, option, value):
    if option and value:
        args.extend([option, value])


def _include_flags(env: Environment):
    prefix = env.subst('$CPLINCPREFIX')
    suffix = env.subst('$CPLINCSUFFIX')
    includes = []
    seen = set()

    for include_dir in _split(env, env.get('CPLPATH', [])) + _split(env, env.get('CPPPATH', [])):
        if include_dir in seen:
            continue
        seen.add(include_dir)
        includes.append(f'{prefix}{include_dir}{suffix}')

    return includes


def _tool_flags(env: Environment):
    args = []

    _append_option(args, env.subst('$CPLASOPTION'), env.subst('$AS'))
    _append_option(args, env.subst('$CPLASFLAGSOPTION'), ' '.join(_split(env, env.get('ASFLAGS', []))))
    _append_option(args, env.subst('$CPLLDOPTION'), env.subst('$LD'))
    _append_option(args, env.subst('$CPLLINKFLAGSOPTION'), ' '.join(_split(env, env.get('LINKFLAGS', []))))

    return args


def _cpl_command(env: Environment, mode_flag, target, source):
    args = [env.subst('$CPL')]
    args.extend(_split(env, env.get('CPLFLAGS', [])))
    args.extend(_include_flags(env))
    args.extend(_tool_flags(env))

    mode = env.subst(mode_flag)
    if mode:
        args.append(mode)

    output_flag = env.subst('$CPLOUTPUTFLAG')
    if output_flag:
        args.extend([output_flag, str(target[0])])
    else:
        args.append(str(target[0]))

    args.extend(str(src) for src in source)
    return args


def _cpl_asm_generator(target, source, env, for_signature):
    return Action(_cpl_command(env, '$CPLEMITASMFLAG', target, source), '$CPLASMCOMSTR')


def _cpl_object_generator(target, source, env, for_signature):
    return Action(_cpl_command(env, '$CPLCOMPILEFLAG', target, source), '$CPLOBJCOMSTR')


def _cpl_program_generator(target, source, env, for_signature):
    return Action(_cpl_command(env, '$CPLLINKFLAG', target, source), '$CPLLINKCOMSTR')


def _cpl_object(env: Environment, source):
    if env.subst('$CPL_OBJECT_MODE') == 'object':
        return env.CPLDirectObject(source)

    asm_sources = env.CPLAsm(source)
    return env.Object(asm_sources)


def setup_cpl_builders(env: Environment):
    env.SetDefault(
        CPL='cpl',
        CPLFLAGS=[],
        CPLPATH=[],
        CPL_OBJECT_MODE='asm',
        CPLEMITASMFLAG='--emit-asm',
        CPLCOMPILEFLAG='--compile',
        CPLLINKFLAG='--link',
        CPLOUTPUTFLAG='-o',
        CPLINCPREFIX='-I',
        CPLINCSUFFIX='',
        CPLASOPTION='--assembler',
        CPLASFLAGSOPTION='--assembler-flags',
        CPLLDOPTION='--linker',
        CPLLINKFLAGSOPTION='--linker-flags',
        CPLASMCOMSTR='CPL -> asm [$SOURCE]',
        CPLOBJCOMSTR='CPL -> obj [$SOURCE]',
        CPLLINKCOMSTR='CPL linking [$TARGET]',
    )

    env.Append(
        BUILDERS={
            'CPLAsm': Builder(
                generator=_cpl_asm_generator,
                suffix='.asm',
                src_suffix='.cpl',
            ),
            'CPLDirectObject': Builder(
                generator=_cpl_object_generator,
                suffix='.o',
                src_suffix='.cpl',
            ),
            'CPLProgram': Builder(
                generator=_cpl_program_generator,
                suffix='.elf',
                src_suffix='.cpl',
            ),
        }
    )

    env.AddMethod(_cpl_object, 'CPLObject')
