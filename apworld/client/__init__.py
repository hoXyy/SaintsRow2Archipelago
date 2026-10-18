from __future__ import annotations

import asyncio

from CommonClient import get_base_parser, gui_enabled, server_loop
from Utils import init_logging

from .context import SR2Context, tracker_loaded

DEFAULT_PLUGIN_PORT = 38282


async def run_client(ctx: SR2Context) -> None:
    await ctx.bridge.start()
    ctx.server_task = asyncio.create_task(server_loop(ctx), name="ServerLoop")
    sync_task = asyncio.create_task(ctx.sync_loop(), name="SR2Sync")
    if tracker_loaded:
        ctx.run_generator()

    if gui_enabled:
        ctx.run_gui()
    ctx.run_cli()

    await ctx.exit_event.wait()
    await ctx.bridge.stop()
    sync_task.cancel()
    await asyncio.gather(sync_task, return_exceptions=True)
    await ctx.shutdown()


def launch(*launch_args: str) -> None:
    parser = get_base_parser(description="Saints Row 2 Archipelago client")
    parser.add_argument(
        "--plugin-port",
        type=int,
        default=DEFAULT_PLUGIN_PORT,
        help="localhost TCP port configured in SR2Archipelago.toml",
    )
    args = parser.parse_args(launch_args)
    init_logging("SR2Client", exception_logger="Client")

    async def main() -> None:
        ctx = SR2Context(args.connect, args.password, args.plugin_port)
        await run_client(ctx)

    asyncio.run(main())
