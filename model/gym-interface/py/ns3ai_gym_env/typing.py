from collections.abc import Callable
from typing import Any, ParamSpec, TypeVar, cast

T = TypeVar("T")
P = ParamSpec("P")


def copy_signature_from(_origin: Callable[P, Any]) -> Callable[[Callable[..., T]], Callable[P, T]]:
    def decorator(target: Callable[..., T]) -> Callable[P, T]:
        return cast(Callable[P, T], target)

    return decorator
